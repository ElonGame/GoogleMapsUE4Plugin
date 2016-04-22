// © 2016 Sam Jeeves. All Rights Reserved.

#include "GoogleMapsPrivatePCH.h"
#include "GoogleMapsGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GenericPlatform/GenericPlatformMath.h"

static AGoogleMapsGameMode* GoogleMapsGameMode_c = NULL;
static bool shouldResumeTracking = false;

void AGoogleMapsGameMode::BeginPlay()
{
	Super::BeginPlay();
	GoogleMapsGameMode_c = this;
}

void AGoogleMapsGameMode::UpdateSplit(float overDistance)
{
	float d = 0;
	int i = GPSPoints.Num() - 1;
	while (d < overDistance && i > 0)
	{
		d += getDistanceFromLatLonInKm(GPSPoints[i].Latitude, GPSPoints[i].Longitude,
			GPSPoints[i - 1].Latitude, GPSPoints[i - 1].Longitude);
		i--;

		//if (i == 0) { // Distance isn't overDistance yet
		//	Split = FTimespan(0);
		//	return;
		//}
	}
	if (d > 0)
		Split = (GPSPoints.Top().Time - GPSPoints[i].Time) * (1 / d);

}

bool AGoogleMapsGameMode::ShouldResumeTracking() const
{
	return shouldResumeTracking;
}

void AGoogleMapsGameMode::ConnectToGoogleAPI()
{
#if PLATFORM_ANDROID
	CallVoidMethodWithExceptionCheck(AndroidThunkJava_ConnectGoogleAPI);
#endif
}

void AGoogleMapsGameMode::DisconnectFromGoogleAPI()
{
#if PLATFORM_ANDROID
	CallVoidMethodWithExceptionCheck(AndroidThunkJava_DisconnectGoogleAPI);
#endif
}


//Privates

void AGoogleMapsGameMode::LocationChanged(float lat, float lng)
{	//First time this is run
	if (!GPSConnected) {
		GPSConnected = true;
		StartTime = FDateTime::Now();
	}

	GPSPoints.Emplace(lat, lng);

	if (GPSPoints.Num() > 1) {
		UpdateSplit(0.2f);
		TotalDistance += getDistanceFromLatLonInKm(GPSPoints.Last(1).Latitude,
			GPSPoints.Last(1).Longitude,
			lat,
			lng);
	}

	// Send location to BP
	OnLocationChanged((float)lat, (float)lng);
}

float AGoogleMapsGameMode::getDistanceFromLatLonInKm(float lat1, float lon1, float lat2, float lon2) {
	float x = (lon2 - lon1)*(PI / 180.f) * FGenericPlatformMath::Cos((lat1 + lat2) * (PI / 180.f) / 2);
	float y = (lat2 - lat1)* (PI / 180.f);
	float R = 6371;
	return FGenericPlatformMath::Sqrt(x*x + y*y) * R;
}


// **** GoogleMaps native functions **** //
#if PLATFORM_ANDROID
extern "C" void Java_com_jeevcatgames_UEMapDialog_nativeLocationChanged(JNIEnv* jenv, jobject thiz, jdouble lat, jdouble lng)
{
	UE_LOG(LogGoogleMaps, Log, TEXT("nativeLocationChanged lat=%.4f and lng=%.4f"), lat, lng);
	GoogleMapsGameMode_c->LocationChanged(lat, lng);
}

extern "C" void Java_com_epicgames_ue4_GameActivity_nativeResumeTracking(JNIEnv* jenv, jobject thiz)
{
	UE_LOG(LogGoogleMaps, Log, TEXT("nativeResumeTracking"));
	shouldResumeTracking = true;
}
#endif
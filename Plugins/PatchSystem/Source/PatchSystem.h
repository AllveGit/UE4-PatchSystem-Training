#pragma once

#include "CoreMinimal.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "PatchSystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPatchCompleteDelegate, bool, bSuccess);

/** ��ġ �ٿ�ε� ���¸� �˷��ִ� �������� ��Ƴ��� ����ü */
USTRUCT(BlueprintType)
struct FPatchDownloadInfo
{
	GENERATED_BODY()

public:
	FPatchDownloadInfo()
	{
	}

	FPatchDownloadInfo(FText InErrorText, int32 InFileDownloaded, int32 InTotalFileDownload, int32 InMBDownloaded, int32 InTotalMBDownload, float InDownloadPer) :
		DownloadErrorText(InErrorText),
		FilesDownloaded(InFileDownloaded),
		TotalFilesToDownload(InTotalFileDownload),
		MBDownloaded(InMBDownloaded),
		TotalMBToDownload(InTotalMBDownload),
		DownloadPercent(InDownloadPer)
	{
	}
public:
	FText DownloadErrorText;

	int32 FilesDownloaded;
	int32 TotalFilesToDownload;

	int32 MBDownloaded;
	int32 TotalMBToDownload;

	float DownloadPercent;
};

UCLASS()
class PATCHSYSTEM_API UPatchSystem : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FString& InPatchPlatform);
	void Shutdown();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/** Patch Readying */
	void OnPatchVersionResponse(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInSuccess);
	void OnPatchManifestUpdateComplete(bool bInSuccess);
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/** Patch Progressing */
	void ProcessPatch();
	void OnPatchMountChunkComplete(bool bInSuccess);
	FPatchDownloadInfo GetPatchDownloadInfo() const;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/** Delegates */
	FPatchCompleteDelegate OnPatchReady;
	FPatchCompleteDelegate OnPatchComplete;
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	/** TODO : �����߰� �ʿ�, �����Ҷ� ini Ű �ӽñ�ӽñ�.. */
	const FString DeploymentName = "Patching-Live";

	/** ���ÿ� ��� �ٿ���� �� �ִ��� */
	const int32 ConcurrentAllowDownloadNum = 8;

	/** ��ġ �Ŵ��佺Ʈ�� ������ Ȯ�εǾ����� Ȯ�� */
	bool bIsDownloadManifestUpToDate;

	/** ��ġ ���������� ���� Ȯ�� */
	bool bIsPatchProcessing;

	/** ��ġ ������ Ȯ���� �����ּ� */
	FString PatchVersionURL;

	/** ��ġ�� �÷��� �̸� (ex. Windows, Android, IOS...) */
	FString PatchPlatform;

	/** �ٿ���� ûũ�� ID�� */
	TArray<int32> ChunkDownloadIDs;
};
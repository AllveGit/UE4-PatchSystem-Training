#include "PatchSystem.h"
#include "ChunkDownloader.h"

DEFINE_LOG_CATEGORY_STATIC(LogPatch, Display, Display);

void UPatchSystem::Initialize(const FString& InPatchPlatform)
{
	PatchPlatform = InPatchPlatform;

	FHttpModule& http = FHttpModule::Get();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> request = http.CreateRequest();
	request->OnProcessRequestComplete().BindUObject(this, &UPatchSystem::OnPatchVersionResponse);

	// configure and send the request
	request->SetURL(PatchVersionURL);
	request->SetURL("GET");
	request->SetHeader(TEXT("User-Agent"), "X-UnrealEngine-Agent");
	request->SetHeader("Content-Type", TEXT("application/json"));
	request->ProcessRequest();
}

void UPatchSystem::Shutdown()
{
	// Shutdown the chunk downloader
	FChunkDownloader::Shutdown();
}

void UPatchSystem::OnPatchVersionResponse(FHttpRequestPtr InRequest, FHttpResponsePtr InResponse, bool bInSuccess)
{
	// ���ο� ��ġ ������ �����κ��� �´ٸ� ������Ʈ ����
	FString contentBuildID = InResponse->GetContentAsString();
	UE_LOG(LogPatch, Display, TEXT("Patch Content Build ID Response : %s"), *contentBuildID);

	// �÷����� �´� ChunkDownloader ���� �� ����
	UE_LOG(LogPatch, Display, TEXT("Creating Chunk Downloader"));
	TSharedRef<FChunkDownloader> downloader = FChunkDownloader::GetOrCreate();
	downloader->Initialize(PatchPlatform, ConcurrentAllowDownloadNum);

	// ĳ�̵� ���� �Ŵ��佺Ʈ �����Ͱ� �ִ� ��� �ε��Ѵ�
	// ChunkDownloader�� ���� �ֱٿ� �ٿ�ε�� �Ŵ��佺Ʈ �����ͷ� ä������
	UE_LOG(LogPatch, Display, TEXT("Loading Cached Build ID"));
	if (downloader->LoadCachedBuild(DeploymentName))
	{
		UE_LOG(LogPatch, Display, TEXT("Cached Build Succeeded"));
	}
	else
	{
		UE_LOG(LogPatch, Display, TEXT("Cached Build Failed"));
	}

	// �Ŵ��佺Ʈ ������Ʈ�� ���� �� ���¸� �����ϴ� �ݹ��Լ�
	TFunction<void(bool)> manifestUpdCompleteCallback = [&](bool bInSuccess) { OnPatchManifestUpdateComplete(bInSuccess); };

	// ������ ���ο� �Ŵ��佺Ʈ ������ �ִ��� Ȯ�� �� �ٿ�ε� �ϴ� �Ŵ��佺Ʈ ������Ʈ ����
	downloader->UpdateBuild(DeploymentName, contentBuildID, manifestUpdCompleteCallback);
}

void UPatchSystem::OnPatchManifestUpdateComplete(bool bInSuccess)
{
	if (bInSuccess)
	{
		UE_LOG(LogPatch, Display, TEXT("Manifest Update Succeeded"));
	}
	else
	{
		UE_LOG(LogPatch, Display, TEXT("Manifest Update Failed"));
	}

	bIsDownloadManifestUpToDate = bInSuccess;

	// ������� �Դٸ�, ��ġ�� �ϱ� ���� �غ� ���� �����̴�.
	OnPatchReady.Broadcast(bInSuccess);
}

void UPatchSystem::ProcessPatch()
{
	// �Ŵ��佺Ʈ�� �ֽ� ���°� �ƴ϶��
	if (!bIsDownloadManifestUpToDate)
	{
		UE_LOG(LogPatch, Display, TEXT("Manifest Update Failed. Can't patch the game"));
		return;
	}

	bIsPatchProcessing = true;

	TSharedRef<FChunkDownloader> downloader = FChunkDownloader::GetChecked();
	
	// ûũ���� ���� (�ٿ�ε�Ǿ�����, ���� ������, ���� ������ ��)�� �α׷� �����.
	for (int32 chunkID : ChunkDownloadIDs)
	{
		int32 chunkStatus = static_cast<int32>(downloader->GetChunkStatus(chunkID));
		UE_LOG(LogPatch, Display, TEXT("chunk %i status: %i"), chunkID, chunkStatus);
	}

	TFunction<void(bool)> mountChunkCompleteCallback = [this](bool bInSuccess) { OnPatchMountChunkComplete(bInSuccess); };
	downloader->MountChunks(ChunkDownloadIDs, mountChunkCompleteCallback);
}

void UPatchSystem::OnPatchMountChunkComplete(bool bInSuccess)
{
	bIsPatchProcessing = false;

	if (bInSuccess)
	{
		UE_LOG(LogPatch, Display, TEXT("Chunk Mount Complete"));
		OnPatchComplete.Broadcast(true);
	}
	else
	{
		UE_LOG(LogPatch, Display, TEXT("Chunk Mount Failed"));
		OnPatchComplete.Broadcast(false);
	}
}

FPatchDownloadInfo UPatchSystem::GetPatchDownloadInfo() const
{
	// ChunkDownloader�� �ٿ�ε� ������ �����´�.
	TSharedRef<FChunkDownloader> downloader = FChunkDownloader::GetChecked();
	FChunkDownloader::FStats loadingStats = downloader->GetLoadingStats();

	// MiB�� ����Ʈ ��
	const uint64 byteFromMiB = (1024 * 1024);

	// �ٿ�ε� ���� ����
	int32 filesDownloaded = (int32)loadingStats.FilesDownloaded;
	int32 totalFilesDownload = (int32)loadingStats.TotalFilesToDownload;
	int32 mbDownloaded = (int32)(loadingStats.BytesDownloaded / byteFromMiB);
	int32 totalMBDownload = (int32)(loadingStats.TotalBytesToDownload / byteFromMiB);
	float downloadPercent = (float)loadingStats.BytesDownloaded / (float)loadingStats.TotalBytesToDownload;

	FPatchDownloadInfo downloadInfo(loadingStats.LastError, filesDownloaded, totalFilesDownload, mbDownloaded, totalMBDownload, downloadPercent);
	return downloadInfo;
}
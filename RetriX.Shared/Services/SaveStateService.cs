﻿using PCLStorage;
using Plugin.LocalNotifications.Abstractions;
using RetriX.Shared.ExtensionMethods;
using System.Threading.Tasks;

namespace RetriX.Shared.Services
{
    public class SaveStateService : ISaveStateService
    {
        private const string SaveStatesFolderName = "SaveStates";

        public const string StateSavedToSlotMessageTitleKey = "StateSavedToSlotMessageTitleKey";
        public const string StateSavedToSlotMessageBodyKey = "StateSavedToSlotMessageBodyKey";

        private readonly ILocalNotifications NotificationService;
        private readonly ILocalizationService LocalizationService;

        private string GameId { get; set; }

        private bool OperationInProgress = false;
        private bool AllowOperations => !(OperationInProgress || SaveStatesFolder == null || GameId == null);

        private IFolder SaveStatesFolder;

        public SaveStateService(ILocalNotifications notificationService, ILocalizationService localizationService)
        {
            NotificationService = notificationService;
            LocalizationService = localizationService;

            GetSubfolderAsync(FileSystem.Current.LocalStorage, SaveStatesFolderName).ContinueWith(d =>
            {
                SaveStatesFolder = d.Result;
            });

            GameId = null;
        }

        public void SetGameId(string id)
        {
            GameId = null;
            if(string.IsNullOrEmpty(id) || string.IsNullOrWhiteSpace(id))
            {
                return;
            }

            GameId = id.MD5();
        }

        public async Task<byte[]> LoadStateAsync(uint slotId)
        {
            if (!AllowOperations)
            {
                return null;
            }

            OperationInProgress = true;

            var statesFolder = await GetGameSaveStatesFolderAsync();
            var fileName = GenerateSaveFileName(slotId);
            var fileExistence = await statesFolder.CheckExistsAsync(fileName);
            if (fileExistence == ExistenceCheckResult.NotFound)
            {
                OperationInProgress = false;
                return null;
            }

            var file = await statesFolder.GetFileAsync(fileName);
            using (var stream = await file.OpenAsync(FileAccess.Read))
            {
                var output = new byte[stream.Length];
                await stream.ReadAsync(output, 0, output.Length);

                OperationInProgress = false;
                return output;
            }
        }

        public async Task<bool> SaveStateAsync(uint slotId, byte[] data)
        {
            if (!AllowOperations)
            {
                return false;
            }

            OperationInProgress = true;

            var statesFolder = await GetGameSaveStatesFolderAsync();
            var fileName = GenerateSaveFileName(slotId);
            var file = await statesFolder.CreateFileAsync(fileName, CreationCollisionOption.ReplaceExisting);
            using (var stream = await file.OpenAsync(FileAccess.ReadAndWrite))
            {
                await stream.WriteAsync(data, 0, data.Length);
            }

            var messageTitle = LocalizationService.GetLocalizedString(StateSavedToSlotMessageTitleKey);
            var messageBody = LocalizationService.GetLocalizedString(StateSavedToSlotMessageBodyKey);
            messageBody = string.Format(messageBody, slotId);
            NotificationService.Show(messageTitle, messageBody);

            OperationInProgress = false;
            return true;
        }

        public async Task<bool> SlotHasDataAsync(uint slotId)
        {
            if (!AllowOperations)
            {
                return false;
            }

            OperationInProgress = true;

            var statesFolder = await GetGameSaveStatesFolderAsync();
            var fileName = GenerateSaveFileName(slotId);
            var result = await statesFolder.CheckExistsAsync(fileName);

            OperationInProgress = false;
            return result == ExistenceCheckResult.FileExists;
        }

        public async Task ClearSavesAsync()
        {
            if (!AllowOperations)
            {
                return;
            }

            OperationInProgress = true;

            var statesFolder = await GetGameSaveStatesFolderAsync();
            await statesFolder.DeleteAsync();
            await GetGameSaveStatesFolderAsync();

            OperationInProgress = false;
        }

        private string GenerateSaveFileName(uint slotId)
        {
            return $"{GameId}_S{slotId}.sav";
        }

        private Task<IFolder> GetGameSaveStatesFolderAsync()
        {
            return GetSubfolderAsync(SaveStatesFolder, GameId);
        }

        private static async Task<IFolder> GetSubfolderAsync(IFolder parent, string name)
        {
            IFolder output;
            var folderExistence = await parent.CheckExistsAsync(name);
            if (folderExistence == ExistenceCheckResult.FolderExists)
            {
                output = await parent.GetFolderAsync(name);
            }
            else
            {
                output = await parent.CreateFolderAsync(name, CreationCollisionOption.ReplaceExisting);
            }

            return output;
        }
    }
}

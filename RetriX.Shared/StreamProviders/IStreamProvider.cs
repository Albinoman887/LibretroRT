﻿using System;
using System.Collections.Generic;
using System.IO;
using System.Threading.Tasks;

namespace RetriX.Shared.StreamProviders
{
    public interface IStreamProvider : IDisposable
    {
        Task<IEnumerable<string>> ListEntriesAsync();
        Task<Stream> GetFileStreamAsync(string path, PCLStorage.FileAccess accessType);
    }
}
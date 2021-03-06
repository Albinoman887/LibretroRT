﻿using System.Threading.Tasks;
using Xunit;

namespace LibretroRT.Test.Cores
{
    public class FCEUMMCoreTest : TestBase
    {
        protected const string RomName = "Super Mario Bros 3.nes";

        public FCEUMMCoreTest() : base(() => FCEUMMRT.FCEUMMCore.Instance)
        {
        }

        [Theory]
        [InlineData(RomName)]
        public override Task LoadingRomWorks(string romName)
        {
            return LoadingRomWorksInternal(romName);
        }

        [Theory]
        [InlineData(RomName)]
        public override Task ExecutionWorks(string romName)
        {
            return ExecutionWorksInternal(romName);
        }
    }
}

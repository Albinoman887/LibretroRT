#pragma once

#include "../LibretroRT_Tools/CoreBase.h"

using namespace Platform;
using namespace LibretroRT_Tools;
using namespace Windows::Storage;

namespace VBAMRT
{
	private ref class VBAMCoreInternal sealed : public CoreBase
	{
	protected private:
		VBAMCoreInternal();

	public:
		static property VBAMCoreInternal^ Instance { VBAMCoreInternal^ get(); }
		virtual ~VBAMCoreInternal();
	};
}
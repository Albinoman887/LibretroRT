#pragma once

namespace LibretroRT_FrontendComponents_AngleRenderer
{
	public interface class IRenderer
	{
		void Init();
		void Deinit();

		void Draw();
		void UpdateWindowSize(GLsizei width, GLsizei height);
	};
}


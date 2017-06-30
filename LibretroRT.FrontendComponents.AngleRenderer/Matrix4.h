#pragma once

namespace LibretroRT_FrontendComponents_AngleRenderer
{
	struct Matrix4
	{
	private:
		Matrix4(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33);

	public:
		float m[4][4];

		static Matrix4 Identity();
		static Matrix4 ScaleMatrix(float x, float y, float z);
		static Matrix4 TranslateMatrix(float x, float y, float z);
		static Matrix4 TransScaleMatrix(float sx, float sy, float sz, float tx, float ty, float tz);
	};
}

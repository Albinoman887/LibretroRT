#pragma once

namespace LibretroRT_FrontendComponents_AngleRenderer
{
	namespace MathHelper
	{
		struct Matrix4
		{
			Matrix4(float m00, float m01, float m02, float m03,
				float m10, float m11, float m12, float m13,
				float m20, float m21, float m22, float m23,
				float m30, float m31, float m32, float m33)
			{
				m[0][0] = m00; m[0][1] = m01; m[0][2] = m02; m[0][3] = m03;
				m[1][0] = m10; m[1][1] = m11; m[1][2] = m12; m[1][3] = m13;
				m[2][0] = m20; m[2][1] = m21; m[2][2] = m22; m[2][3] = m23;
				m[3][0] = m30; m[3][1] = m31; m[3][2] = m32; m[3][3] = m33;
			}

			float m[4][4];
		};

		inline static Matrix4 ScaleMatrix(float x, float y, float z)
		{
			return Matrix4(x, 0.0f, 0.0f, 0.0f,
				0.0f, y, 0.0f, 0.0f,
				0.0f, 0.0f, z, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);
		}

		inline static Matrix4 TranslateMatrix(float x, float y, float z)
		{
			return Matrix4(0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f,
				x, y, z, 1.0f);
		}

		inline static Matrix4 TransScaleMatrix(float sx, float sy, float sz, float tx, float ty, float tz)
		{
			return Matrix4(sx, 0.0f, 0.0f, 0.0f,
				0.0f, sy, 0.0f, 0.0f,
				0.0f, 0.0f, sz, 0.0f,
				tx, ty, tz, 1.0f);
		}
	}
}

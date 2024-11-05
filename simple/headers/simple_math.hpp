#pragma once

#include "simple_algorithm.hpp"
#include <math.h>
#include <cmath>
#include <cstdint>

namespace simple {

	constexpr auto PI = 3.14159265358979323846;

	struct IntVec2;
	struct Vec3;
	struct Vec4;
	struct Quaternion;
	struct Matrix4x4;
	struct Matrix3x3;
	struct Matrix2x2;

	struct Vec2 {

		float x, y;

		inline Vec2(float x = 0.0f, float y = 0.0f) noexcept : x(x), y(y) {}

		inline float SqrMagnitude() const noexcept { return x * x + y * y; }
		inline float Magnitude() const noexcept { return sqrt(SqrMagnitude()); }
		inline Vec2 Normalized() const noexcept { 
			float mag = SqrMagnitude();
			if (mag < 0.00001f) {
				return Vec2(0.0f, 0.0f);
			}
			mag = sqrt(mag);
			return Vec2(x / mag, y / mag);
		}

		inline Vec2 operator*(float scalar) const noexcept { return Vec2(x * scalar, y * scalar); }
		inline Vec2 operator/(float scalar) const noexcept { return Vec2(x * scalar, y / scalar); }
		inline Vec2 operator-() const noexcept { return Vec2(-x, -y); }
		inline Vec2 operator+(Vec2 other) const noexcept { return Vec2(x + other.x, y + other.y); }
		inline Vec2 operator-(Vec2 other) const noexcept { return Vec2(x - other.x, y - other.y); }
		inline Vec2& operator+=(Vec2 other) noexcept { x += other.x; y += other.y; return *this; }
	};

	struct DVec2 {
		double x, y;
	};

	struct IntVec2 {

		int x, y;

		inline IntVec2(int x = 0, int y = 0) noexcept : x(x), y(y) {}

		inline bool operator!=(IntVec2 other) const noexcept { return x != other.x && y != other.y; }

		inline IntVec2 operator*(float scalar) const noexcept { return IntVec2(x * scalar, y * scalar); }
		inline IntVec2 operator/(float scalar) const noexcept { return IntVec2(x / scalar, y / scalar); }
		inline IntVec2 operator+(IntVec2 other) const noexcept { return IntVec2(x + other.x, y + other.y); }
		inline IntVec2 operator-(IntVec2 other) const noexcept { return IntVec2(x - other.x, y - other.y); }
		inline IntVec2& operator+=(IntVec2 other) noexcept { x += other.x; y += other.y; return *this; }

		inline explicit operator Vec2() const noexcept { return Vec2(x, y); }
	};

	struct Vec3 {

		float x, y, z;

		inline Vec3(float x = 0.0f, float y = 0.0f, float z = 0.0f) noexcept : x(x), y(y), z(z) {}
		inline Vec3(const Vec2& other) noexcept : x(other.x), y(other.y), z(0.0f) {}

		static inline float Dot(const Vec3& a, const Vec3& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
		static inline Vec3 Cross(const Vec3& a, const Vec3& b) noexcept { return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
		static inline void Normalize(Vec3& vec) noexcept {
			float mag = vec.SqrMagnitude();
			if (mag == 0.0f) {
				vec.x = 0.0f; vec.y = 0.0f; vec.z = 0.0f;
				return;
			}
			vec.x /= mag; vec.y /= mag; vec.z /= mag;
		}

		inline float SqrMagnitude() const noexcept { return x * x + y * y + z * z; }
		inline float Magnitude() const noexcept { return sqrt(SqrMagnitude()); }
		inline Vec3 Normalized() const noexcept {
			float mag = SqrMagnitude();
			if (mag < 0.00001f) {
				return Vec3(0.0f, 0.0f, 0.0f);
			}
			mag = sqrt(mag);
			return Vec3(x / mag, y / mag, z / mag);
		}

		inline Vec3 operator*(float scalar) const noexcept { return Vec3(x * scalar, y * scalar, z * scalar); }
		inline Vec3 operator/(float scalar) const noexcept { return Vec3(x / scalar, y / scalar, z / scalar); }
		inline Vec3 operator-() const noexcept { return Vec3(-x, -y, -z); }
		inline Vec3 operator+(const Vec3& other) const noexcept { return Vec3(x + other.x, y + other.y, z + other.z); }
		inline Vec3 operator-(const Vec3& other) const noexcept { return Vec3(x - other.x, y - other.y, z - other.z); }
		inline Vec3& operator+=(const Vec3& other) noexcept { x += other.x; y += other.y; z += other.z; return *this; }
		inline Vec3& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }

		inline bool operator==(const Vec3& other) const noexcept { return x == other.x && y == other.y && z == other.z; }

		explicit operator Vec4() const noexcept;
		inline explicit operator Vec2() const noexcept { return Vec2(x, y); }
	};

	struct IntVec3 {
		int x, y, z;
	};

	struct Vec4 {

		float x, y, z, w;

		inline Vec4(float x = 0.0f, float y = 0.0f, float z = 0.0f, float w = 0.0f) noexcept : x(x), y(y), z(z), w(w) {}
		inline Vec4(const Vec3& other, float w = 0.0f) noexcept : x(other.x), y(other.y), z(other.z), w(w) {}

		static inline float Dot(const Vec4& a, const Vec4& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

		inline float SqrMagnitude() const noexcept { return x * x + y * y + z * z + w * w; }
		inline float Magnitude() const noexcept { return sqrt(SqrMagnitude()); }
		inline Vec4 Normalized() const noexcept {
			float mag = SqrMagnitude();
			if (mag < 0.00001f) {
				return { 0.0f, 0.0f, 0.0f, 0.0f };
			}
			mag = sqrtf(mag);
			return { x / mag, y / mag, z / mag, w / mag };
		}

		inline Vec4 operator*(float scalar) const noexcept { return Vec4(x * scalar, y * scalar, z * scalar, w * scalar); }
		inline Vec4& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; w *= scalar; return *this; }

		inline explicit operator Vec3() const noexcept { return Vec3(x, y, z); }
		inline explicit operator Vec2() const noexcept { return Vec2(x, y); }
	};

	struct IntVec4 {
		int x, y, z, w;
	};

	struct Matrix2x2 {
		Vec2 column0;
		Vec2 column1;
		inline Matrix2x2() noexcept : column0(1.0f, 0.0f), column1(0.0f, 1.0f) {}
		inline Matrix2x2(float num0, float num1, float num2, float num3) noexcept : column0(num0, num1), column1(num2, num3) {}
		inline Matrix2x2(Vec2 column0, Vec2 column1) noexcept : column0(column0), column1(column1) {}
		static inline float Determinant(const Matrix2x2& a) noexcept { return a.column0.x * a.column1.y - a.column0.y * a.column1.x; }
	};
	
	struct Matrix3x3 {

		Vec3 column0;
		Vec3 column1;
		Vec3 column2;

		inline Matrix3x3() noexcept : column0(1.0f, 0.0f, 0.0f), column1(0.0f, 1.0f, 0.0f), column2(0.0f, 0.0f, 1.0f) {}
		inline Matrix3x3(const Vec3& column0, const Vec3& column1, const Vec3& column2) noexcept
			: column0(column0), column1(column1), column2(column2) {}
		inline Matrix3x3(float n0, float n1, float n2, float n3, float n4, float n5, float n6, float n7, float n8) noexcept
			: column0(n0, n1, n2), column1(n3, n4, n5), column2(n6, n7, n8) {}

		static inline Matrix3x3& Transpose(Matrix3x3& a) noexcept {
			simple::Swap(a.column0.z, a.column2.x);
			simple::Swap(a.column0.y, a.column1.x);
			simple::Swap(a.column1.z, a.column2.y);
			return a;
		}
		static inline Matrix3x3& Inverse(Matrix3x3& a) noexcept {
			Vec3 minors0 = Vec3(
				Matrix2x2::Determinant(Matrix2x2(a.column1.y, a.column1.z, a.column2.y, a.column2.z)),
				Matrix2x2::Determinant(Matrix2x2(a.column1.z, a.column1.x, a.column2.z, a.column2.x)),
				Matrix2x2::Determinant(Matrix2x2(a.column1.x, a.column1.y, a.column2.x, a.column2.y))
			);
			Vec3 minors1 = Vec3(
				Matrix2x2::Determinant(Matrix2x2(a.column2.y, a.column2.z, a.column0.y, a.column0.z)),
				Matrix2x2::Determinant(Matrix2x2(a.column2.z, a.column2.x, a.column0.z, a.column0.x)),
				Matrix2x2::Determinant(Matrix2x2(a.column2.x, a.column2.y, a.column0.x, a.column0.y))
			);
			Vec3 minors2 = Vec3(
				Matrix2x2::Determinant(Matrix2x2(a.column0.y, a.column0.z, a.column1.y, a.column1.z)),
				Matrix2x2::Determinant(Matrix2x2(a.column0.z, a.column0.x, a.column1.z, a.column1.x)),
				Matrix2x2::Determinant(Matrix2x2(a.column0.x, a.column0.y, a.column1.x, a.column1.y))
			);
			a = { minors0, minors1, minors2 };
			return Transpose(a);
		}
	};

	struct Matrix4x4 {

		Vec4 column0;
		Vec4 column1;
		Vec4 column2;
		Vec4 column3;

		inline Matrix4x4() noexcept
			: column0(1.0f, 0.0f, 0.0f, 0.0f), column1(0.0f, 1.0f, 0.0f, 0.0f),
			column2(0.0f, 0.0f, 1.0f, 0.0f), column3(0.0f, 0.0f, 0.0f, 1.0f) {}
		inline Matrix4x4(const Vec4& column0, const Vec4& column1, const Vec4& column2, const Vec4& column3) noexcept : column0(column0), column1(column1), column2(column2), column3(column3) {}
		inline Matrix4x4(float n0, float n1, float n2, float n3, float n4, float n5, float n6, float n7, float n8, float n9, float n10, float n11, float n12, float n13, float n14, float n15) noexcept
		 : column0(n0, n1, n2, n3), column1(n4, n5, n6, n7),
			column2(n8, n9, n10, n11), column3(n12, n13, n14, n15) {}

		static inline Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b) noexcept {
			Matrix4x4 result{};
			float (*aF)[4] = (float(*)[4])&a;
			float (*bF)[4] = (float(*)[4])&b;
			float (*resF)[4] = (float(*)[4])&result;
			for (int i = 0; i < 4; i++) {
				float num = 0.0f;
				for (int j = 0; j < 4; j++) {
					for (int k = 0; k < 4; k++) {
						num += aF[k][j] * bF[i][k];
					}
					resF[i][j] += num;
				}
			}
			return result;
		}

		static inline Vec4 Multiply(const Matrix4x4& a, const Vec4& b) noexcept {
			return Vec4(
				a.column0.x * b.x + a.column1.x * b.y + a.column2.x * b.z + a.column3.x * b.w,
				a.column0.y * b.x + a.column1.y * b.y + a.column2.y * b.z + a.column3.y * b.w,
				a.column0.z * b.x + a.column1.z * b.y + a.column2.z * b.z + a.column3.z * b.w,
				a.column0.w * b.x + a.column1.w * b.y + a.column2.w * b.z + a.column3.w * b.w);
		}

		static inline Vec4 Multiply(const Vec4& a, const Matrix4x4& b) noexcept {
			return Vec4(
				a.x * b.column0.x + a.y * b.column0.y + a.z * b.column0.z + a.w * b.column0.w,
				a.x * b.column1.x + a.y * b.column1.y + a.z * b.column1.z + a.w * b.column1.w,
				a.x * b.column2.x + a.y * b.column2.y + a.z * b.column2.z + a.w * b.column2.w,
				a.x * b.column3.x + a.y * b.column3.y + a.z * b.column3.z + a.w * b.column3.w);
		}

		static inline float Determinant(const Matrix4x4& a) noexcept {
			return
				a.column0.x * a.column1.y * a.column2.z * a.column3.w +
				a.column0.x * a.column1.z * a.column2.w * a.column3.y +
				a.column0.x * a.column1.w * a.column2.y * a.column3.z -
				a.column0.x * a.column1.w * a.column2.z * a.column3.y -
				a.column0.x * a.column1.z * a.column2.y * a.column3.w -
				a.column0.x * a.column1.y * a.column2.w * a.column3.z -
				a.column0.y * a.column1.x * a.column2.z * a.column3.w -
				a.column0.z * a.column1.x * a.column2.w * a.column3.y -
				a.column0.w * a.column1.x * a.column2.y * a.column3.z +
				a.column0.w * a.column1.x * a.column2.z * a.column3.y +
				a.column0.z * a.column1.x * a.column2.y * a.column3.w +
				a.column0.y * a.column1.x * a.column2.w * a.column3.z +
				a.column0.y * a.column1.z * a.column2.x * a.column3.w +
				a.column0.z * a.column1.w * a.column2.x * a.column3.y +
				a.column0.w * a.column1.y * a.column2.x * a.column3.z -
				a.column0.w * a.column1.z * a.column2.x * a.column3.y -
				a.column0.z * a.column1.y * a.column2.x * a.column3.w -
				a.column0.y * a.column1.w * a.column2.x * a.column3.z -
				a.column0.y * a.column1.z * a.column2.w * a.column3.x -
				a.column0.z * a.column1.w * a.column2.y * a.column3.x -
				a.column0.w * a.column1.y * a.column2.z * a.column3.x +
				a.column0.w * a.column1.z * a.column2.y * a.column3.x +
				a.column0.z * a.column1.y * a.column2.w * a.column3.x +
				a.column0.y * a.column1.w * a.column2.z * a.column3.x;
		}

		static inline Matrix4x4 LookAt(const Vec3& eyePosition, const Vec3& upDirection, const Vec3& lookAtPosition) noexcept {
			Vec3 pos = eyePosition;
			pos.y = -pos.y;
			Vec3 front = (lookAtPosition - pos).Normalized();
			Vec3 right = Vec3::Cross(upDirection.Normalized(), front).Normalized();
			Vec3 up = Vec3::Cross(front, right).Normalized();
			Matrix4x4 result{};
			result.column0.x = right.x;
			result.column1.x = right.y;
			result.column2.x = right.z;
			result.column0.y = up.x;
			result.column1.y = up.y;
			result.column2.y = up.z;
			result.column0.z = front.x;
			result.column1.z = front.y;
			result.column2.z = front.z;
			pos = -pos;
			result.column3 = {
				Vec3::Dot(right, pos),
				Vec3::Dot(up, pos),
				Vec3::Dot(front, pos),
				1.0f
			};
			return result;
		}

		static inline Matrix4x4 Projection(float radFovY, float aspectRatio, float zNear, float zFar) noexcept {
			float halfTan = tan(radFovY / 2);
			Matrix4x4 result{};
			result.column0.x = 1 / (aspectRatio * halfTan);
			result.column1.y = 1 / halfTan;
			result.column2.z = (zFar + zNear) / (zFar - zNear);
			result.column2.w = 1;
			result.column3.z = (2 * zFar * zNear) / (zFar - zNear);
			return result;
		}

		static inline Matrix4x4 Orthogonal(float leftPlane, float rightPlane, float bottomPlane, float topPlane, float nearPlane, float farPlane) noexcept {
			Matrix4x4 result{};
			result.column0.x = 2.0f / (rightPlane - leftPlane);
			result.column1.y = 2.0f / (topPlane - bottomPlane);
			result.column2.z = 2.0f / (nearPlane + farPlane);
			result.column3.x = -(rightPlane + leftPlane) / (rightPlane - leftPlane);
			result.column3.y = -(bottomPlane + topPlane) / (bottomPlane - topPlane);
			result.column3.z = nearPlane / (nearPlane + farPlane);
			return result;
		}

		static inline Matrix4x4 Transpose(const Matrix4x4& a) noexcept {
			Matrix4x4 result = a;
			simple::Swap(result.column0.y, result.column1.x);
			simple::Swap(result.column0.z, result.column2.x);
			simple::Swap(result.column0.w, result.column3.x);
			simple::Swap(result.column1.z, result.column3.y);
			simple::Swap(result.column3.y, result.column1.w);
			simple::Swap(result.column3.z, result.column2.w);
			return result;
		}

		static Matrix4x4& Inverse(Matrix4x4& a) noexcept;

		inline explicit operator Matrix3x3() const noexcept {
			return Matrix3x3((Vec3)column0, (Vec3)column1, (Vec3)column2);
		}
	};

	struct Quaternion {

		float x, y, z, w;

		static inline float Dot(const Quaternion& a, const Quaternion& b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }
		static inline float AngleBetween(const Quaternion& a, const Quaternion& b) noexcept {
			return acos(fmin(fabs(Quaternion::Dot(a, b)), 1.0f)) * 2.0f;
		}
		static inline Quaternion Slerp(const Quaternion& from, const Quaternion& to, float t) noexcept {
			return Quaternion(from * (1 - t) + to * t).Normalized();
		}
		static inline Quaternion RotateTowards(const Quaternion& from, const Quaternion& to, float maxRadians) noexcept {
			float angle = AngleBetween(from, to);
			if (abs(angle) < 0.00001f) {
				return to;
			}
			return Slerp(from, to, Clamp(maxRadians / angle, 0.0f, 1.0f));
		}
		static inline Quaternion AxisRotation(const Vec3& axis, float radians) noexcept {
			radians /= 2;
			Vec3 norm = axis.Normalized();
			float sine = sin(radians);
			return Quaternion(norm.x * sine, norm.y * sine, norm.z * sine, cos(radians)).Normalized();
		}
		static inline Quaternion Multiply(const Quaternion& a, const Quaternion& b) noexcept {
			return Quaternion(
				a.x * b.w + a.w * b.x - a.y * b.z + a.z * b.y,
				a.y * b.w - a.z * b.x + a.w * b.y + a.x * b.z,
				a.z * b.w + a.y * b.x - a.x * b.y + a.w * b.x,
				a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
			).Normalized();
		}
		static inline Quaternion RotationBetween(const Vec3& a, const Vec3& b) noexcept {
			float aLen = a.SqrMagnitude(), bLen = b.SqrMagnitude();
			if (aLen < 0.00001f || bLen < 0.00001f) {
				return { 0, 0, 0, 0 };
			}
			Vec3 abCross = Vec3::Cross(a, b);
			return Quaternion(abCross.x, abCross.y, abCross.z, sqrt(aLen * bLen + Vec3::Dot(a, b))).Normalized();
		}

		inline Quaternion() noexcept : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
		inline Quaternion(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
		inline Quaternion(Vec4 other) noexcept : x(other.x), y(other.y), z(other.z), w(other.w) {}

		inline float SqrMagnitude() const noexcept { return x * x + y * y + z * z + w * w; }
		inline float Magnitude() const noexcept { return sqrt(SqrMagnitude()); }
		inline Quaternion Normalized() const noexcept {
			float mag = SqrMagnitude();
			if (fabs(mag) < 0.00001f) {
				return { 0.0f, 0.0f, 0.0f, 0.0f };
			}
			mag = sqrt(mag);
			return { x / mag, y / mag, z / mag, w / mag };
		}
		inline Matrix4x4 AsMatrix4x4() const noexcept {
			float num1 = x * x;
			float num2 = y * y;
			float num3 = z * z;
			return Matrix4x4(
				1.0f - 2.0f * (num2 + num3), 2.0f * (x * y + z * w), 2.0f * (x * z - y * w), 0.0f,
				2.0f * (x * y - z * w), 1.0f - 2.0f * (num1 + num3), 2.0f * (y * z + x * w), 0.0f,
				2.0f * (x * z + y * w), 2.0f * (y * z - x * w), 1.0f - 2.0f * (num1 + num2), 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f);
		}

		static inline Quaternion Identity() noexcept { return Quaternion(); }

		Quaternion operator+(const Quaternion& other) const noexcept { return Quaternion(x + other.x, y + other.y, z + other.z, w + other.w); }
		Quaternion operator*(float scalar) const noexcept { return Quaternion(x * scalar, y * scalar, z * scalar, w * scalar); }
		inline bool operator==(const Quaternion& other) const noexcept { return x == other.x && y == other.y && z == other.z && w == other.w; }
		
		inline explicit operator Vec4() const noexcept {
			return Vec4(x, y, z, w);
		}
	};

	inline float Heron (float lenA, float lenB, float lenC) noexcept {
		float s = (lenA + lenB + lenC) / 2;
		return sqrt(s * (s - lenA) * (s - lenB) * (s - lenC));
	}
}

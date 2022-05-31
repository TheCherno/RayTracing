#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include <limits>
#include <cmath>

using namespace Walnut;

struct Vector3
{
	float X;
	float Y;
	float Z;

	Vector3()
		: X(0), Y(0), Z(0)
	{
	}

	Vector3(float d)
		: X(d), Y(d), Z(d)
	{
	}

	Vector3(float x, float y, float z)
		: X(x), Y(y), Z(z)
	{
	}

	Vector3& operator*=(const Vector3& v)
	{
		X *= v.X;
		Y *= v.Y;
		Z *= v.Z;
		return *this;
	}

	Vector3& operator/=(const Vector3& v)
	{
		X /= v.X;
		Y /= v.Y;
		Z /= v.Z;
		return *this;
	}

	Vector3& operator+=(const Vector3& v)
	{
		X += v.X;
		Y += v.Y;
		Z += v.Z;
		return *this;
	}

	Vector3& operator-=(const Vector3& v)
	{
		X -= v.X;
		Y -= v.Y;
		Z -= v.Z;
		return *this;
	}

	Vector3 operator-() const { return { -X, -Y, -Z }; }

	float Length() const { return std::sqrt(LengthSquared()); }
	float LengthSquared() const { return X * X + Y * Y + Z * Z; }
};

inline Vector3 operator+(const Vector3& a, const Vector3& b) { return { a.X + b.X, a.Y + b.Y, a.Z + b.Z }; }
inline Vector3 operator-(const Vector3& a, const Vector3& b) { return { a.X - b.X, a.Y - b.Y, a.Z - b.Z }; }
inline Vector3 operator*(const Vector3& a, const Vector3& b) { return { a.X * b.X, a.Y * b.Y, a.Z * b.Z }; }
inline Vector3 operator/(const Vector3& a, const Vector3& b) { return { a.X / b.X, a.Y / b.Y, a.Z / b.Z }; }

inline Vector3 operator+(const Vector3& a, float b) { return { a.X + b, a.Y + b, a.Z + b }; }
inline Vector3 operator-(const Vector3& a, float b) { return { a.X - b, a.Y - b, a.Z - b }; }
inline Vector3 operator*(const Vector3& a, float b) { return { a.X * b, a.Y * b, a.Z * b }; }
inline Vector3 operator/(const Vector3& a, float b) { return { a.X / b, a.Y / b, a.Z / b }; }

inline Vector3 operator+(float a, const Vector3& b) { return { a + b.X, a + b.Y, a + b.Z }; }
inline Vector3 operator-(float a, const Vector3& b) { return { a - b.X, a - b.Y, a - b.Z }; }
inline Vector3 operator*(float a, const Vector3& b) { return { a * b.X, a * b.Y, a * b.Z }; }
inline Vector3 operator/(float a, const Vector3& b) { return { a / b.X, a / b.Y, a / b.Z }; }

inline float Dot(const Vector3& a, const Vector3& b)
{
	return a.X * b.X
		+ a.Y * b.Y
		+ a.Z * b.Z;
}

inline Vector3 Cross(const Vector3& a, const Vector3& b)
{
	return {
		a.Y * b.Z - a.Z * b.Y,
		a.Z * b.X - a.X * b.Z,
		a.X * b.Y - a.Y * b.X
	};
}

inline Vector3 Normalize(Vector3 v) { return v / v.Length(); }

using Color = Vector3;

struct Ray
{
	Vector3 Origin;
	Vector3 Direction;

	Vector3 At(float t) const
	{
		return Origin + t * Direction;
	}
};

class ExampleLayer : public Layer
{
public:
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Last render: %.3fms", m_LastRenderTime);
		ImGui::Checkbox("Flip Y", &m_FlipY);
		ImGui::DragFloat3("Sphere Center", &(m_SphereCenter.X), 0.01f);
		ImGui::DragFloat("Sphere Radius", &m_SphereRadius, 0.01f, 0.0f, std::numeric_limits<float>::max());
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;
		m_AspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;

		if (m_Image)
			ImGui::Image(m_Image->GetDescriptorSet(), { (float)m_Image->GetWidth(), (float)m_Image->GetHeight() });

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}

	float HitSphere(const Vector3& center, float radius, const Ray& ray) {
		Vector3 oc = ray.Origin - center;
		float a = ray.Direction.LengthSquared();
		float halfB = Dot(oc, ray.Direction);
		float c = oc.LengthSquared() - radius * radius;
		float discriminant = halfB * halfB - a * c;
		if (discriminant < 0.0f)
			return -1.0f;
		return (-halfB - std::sqrt(discriminant)) / a;
	}

	Color RayColor(const Ray& ray)
	{
		float t = HitSphere(m_SphereCenter, m_SphereRadius, ray);
		if (t > 0.0f)
		{
			Vector3 N = Normalize(ray.At(t) - m_SphereCenter);
			return 0.5f * (N + 1);
		}

		Vector3 direction = Normalize(ray.Direction);
		t = 0.5 * (direction.Y + 1.0f);
		return (1.0f - t) * Color(1.0f, 1.0f, 1.0f) + t * Color(0.5f, 0.7f, 1.0f);
	}

	void Render()
	{
		Timer timer;

		if (!m_Image || m_ViewportWidth != m_Image->GetWidth() || m_ViewportHeight != m_Image->GetHeight())
		{
			m_Image = std::make_shared<Image>(m_ViewportWidth, m_ViewportHeight, ImageFormat::RGBA);
			delete[] m_ImageData;
			m_ImageData = new uint32_t[m_ViewportWidth * m_ViewportHeight];
		}

		// Camera
		float viewportHeight = 2.0f;
		float viewportWidth = m_AspectRatio * viewportHeight;
		float focalLength = 1.0f;

		Vector3 origin = { 0, 0, 0 };
		Vector3 horizontal = { (float)viewportWidth, 0, 0 };
		Vector3 vertical = { 0, (float)viewportHeight, 0 };
		Vector3 lowerLeftCorner = origin - horizontal * 0.5f - vertical * 0.5f - Vector3(0, 0, focalLength);

		for (uint32_t x = 0; x < m_ViewportWidth; x++)
		{
			for (uint32_t y = 0; y < m_ViewportHeight; y++)
			{
				float u = (float)x / (m_ViewportWidth - 1);
				float v = (float)y / (m_ViewportHeight - 1);

				Ray ray = { origin, lowerLeftCorner + u * horizontal + v * vertical - origin };
				Color color = RayColor(ray);
				WriteColor(color, x, y);
			}
		}

		m_Image->SetData(m_ImageData);

		m_LastRenderTime = timer.ElapsedMillis();
	}

	uint32_t RGBA_To_ABGR_Hex(const Color& color) const
	{
		uint32_t r = (uint32_t)(color.X * 255.0f + 0.5f);
		uint32_t g = (uint32_t)(color.Y * 255.0f + 0.5f);
		uint32_t b = (uint32_t)(color.Z * 255.0f + 0.5f);
		uint32_t a = (uint32_t)(255.0f + 0.5f);

		return (a << 24) | (b << 16) | (g << 8) | r;
	}

	void WriteColor(const Color& color, uint32_t x, uint32_t y)
	{
		y = m_FlipY ? y : (m_ViewportHeight - 1) - y;
		m_ImageData[x + y * m_ViewportWidth] = RGBA_To_ABGR_Hex(color);
	}
private:
	std::shared_ptr<Image> m_Image;
	uint32_t* m_ImageData = nullptr;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_AspectRatio = 0.0f;

	Vector3 m_SphereCenter = { 0.0f, 0.0f, -1.0f };
	float m_SphereRadius = 0.5f;

	bool m_FlipY = false;
	float m_LastRenderTime = 0.0f;
};

Application* Walnut::CreateApplication(int argc, char** argv)
{
	ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Application* app = new Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}

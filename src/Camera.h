#pragma once

namespace svr
{
	class Camera
	{
	private:
		static constexpr glm::vec3 UP = { 0.0f, 1.0f, 0.0f };
		static constexpr float NEAR = 0.1f;
		static constexpr float FAR = 100.0f;

		glm::vec3 m_pos;
		glm::vec3 m_lookingAt;
		float m_fov;
		float m_aspect;

	public:
		Camera()
			: m_pos(0.0f),
			m_lookingAt(0.0f),
			m_fov(0.0f),
			m_aspect(0.0f)
		{ }

		const glm::vec3& getPos() const { return m_pos; }
		void setPos(const glm::vec3& pos) { m_pos = pos; }
		void setPos(float x, float y, float z) { m_pos = { x, y, z }; }

		const glm::vec3& getLookingAt() const { return m_lookingAt; }
		void setLookingAt(const glm::vec3& lookingAt) { m_lookingAt = lookingAt; }
		void setLookingAt(float x, float y, float z) { m_lookingAt = { x, y, z }; }

		float getFov() const { return m_fov; }
		void setFov(float fov) { m_fov = fov; }

		float getAspectRatio() const { return m_aspect; }
		void setAspectRatio(float aspect) { m_aspect = aspect; }

		glm::mat4 getViewProjection() const
		{
			glm::mat4 view = glm::lookAtRH(m_pos, m_lookingAt, UP);
			glm::mat4 proj = glm::perspective(m_fov, m_aspect, 0.1f, 100.0f);

			return proj * view;
		}
	};
}
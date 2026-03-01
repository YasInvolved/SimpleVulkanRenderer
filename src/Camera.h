// MIT License
// 
// Copyright(c) 2026 Jakub Bączyk
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

namespace svr
{
	class Camera
	{
	private:
		static constexpr glm::vec3 UP = { 0.0f, -1.0f, 0.0f };
		static constexpr float NEAR = 0.1f;
		static constexpr float FAR = 100.0f;

		glm::vec3 m_pos;
		glm::vec3 m_lookingAt;
		
		uint32_t m_fov;
		float m_aspect;

	public:
		Camera()
			: m_pos(0.0f),
			m_lookingAt(0.0f),
			m_fov(0u),
			m_aspect(0.0f)
		{ }

		const glm::vec3& getPos() const { return m_pos; }
		void setPos(const glm::vec3& pos) { m_pos = pos; }
		void setPos(float x, float y, float z) { m_pos = { x, y, z }; }

		const glm::vec3& getLookingAt() const { return m_lookingAt; }
		void setLookingAt(const glm::vec3& lookingAt) { m_lookingAt = lookingAt; }
		void setLookingAt(float x, float y, float z) { m_lookingAt = { x, y, z }; }

		uint32_t getFov() const { return m_fov; }
		void setFov(uint32_t fov) { m_fov = fov; }

		float getAspectRatio() const { return m_aspect; }
		void setAspectRatio(float aspect) { m_aspect = aspect; }

		glm::mat4 getViewProjection() const
		{
			glm::mat4 view = glm::lookAtRH(m_pos, m_lookingAt, UP);
			glm::mat4 proj = glm::perspective(glm::radians(static_cast<float>(m_fov)), m_aspect, 0.1f, 100.0f);

			return proj * view;
		}

		void renderImGuiMenu()
		{
			ImGui::BeginChild(
				"camera_menu", 
				ImVec2(0, 0), 
				ImGuiChildFlags_AutoResizeX |
				ImGuiChildFlags_AutoResizeY |
				ImGuiChildFlags_AlwaysAutoResize
			);

			ImGui::Text("Camera");
			ImGui::Separator();
			ImGui::DragFloat3("Position", reinterpret_cast<float*>(&m_pos.x));
			ImGui::DragFloat3("Looking At", reinterpret_cast<float*>(&m_lookingAt.x));

			constexpr uint32_t FOV_DRAG_MIN = 1u;
			constexpr uint32_t FOV_DRAG_MAX = 140u;
			ImGui::DragScalar(
				"FOV", 
				ImGuiDataType_U32, 
				reinterpret_cast<void*>(&m_fov), 
				1.0f, 
				(const void*)&FOV_DRAG_MIN, 
				(const void*)&FOV_DRAG_MAX
			);

			ImGui::EndChild();
		}
	};
}

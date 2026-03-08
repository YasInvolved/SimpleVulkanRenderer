#pragma once

namespace svr
{
	struct alignas(16) Light
	{
		glm::vec4 positionAndRadius; // x,y,z = position, w = color
		glm::vec4 colorAndIntensity; // x,y,z = color, w = intensity

		static inline void renderLightControlsWindow(const std::span<Light> lights)
		{
			static constexpr ImVec2 CHILD_SIZE = ImVec2(0.0f, 0.0f);
			static constexpr ImGuiChildFlags CHILD_FLAGS =
				ImGuiChildFlags_AutoResizeX |
				ImGuiChildFlags_AutoResizeY |
				ImGuiChildFlags_AlwaysAutoResize;


			for (size_t i = 0; i < lights.size(); i++)
			{
				auto& light = lights[i];

				ImGui::Begin("Lights");

				if (ImGui::BeginChild(fmt::format("light_{:d}_controls", i).c_str(), CHILD_SIZE, CHILD_FLAGS))
				{
					ImGui::Text(fmt::format("Light {:d}", i).c_str());
					ImGui::Separator();

					ImGui::DragFloat3(
						"Position",
						reinterpret_cast<float*>(&light.positionAndRadius[0]),
						0.1f
					);

					ImGui::DragFloat(
						"Radius",
						reinterpret_cast<float*>(&light.positionAndRadius[3]),
						0.1f, 0.1f
					);

					ImGui::ColorPicker3(
						"Color",
						reinterpret_cast<float*>(&light.colorAndIntensity[0])
					);

					ImGui::DragFloat(
						"Intensity",
						reinterpret_cast<float*>(&light.colorAndIntensity[3]),
						0.1f, 0.1f
					);

					ImGui::EndChild();
				}

				ImGui::End();
			}
		}
	};
}

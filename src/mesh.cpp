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

#include "Mesh.h"

using namespace svr;

using Vertex = Mesh::Vertex;

bool Vertex::operator==(const Vertex& other) const
{
	if (this->pos == other.pos && this->color == other.color) return true;
	else return false;
}

// hashing logic for vertex
namespace std
{
	template<> struct hash<svr::Mesh::Vertex>
	{
		size_t operator()(svr::Mesh::Vertex const& vertex) const
		{
			size_t seed = 0;
			seed ^= hash<glm::vec3>()(vertex.pos) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash<glm::vec3>()(vertex.normal) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hash<glm::vec3>()(vertex.color) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}

Mesh Mesh::loadObjMesh(const std::string_view path)
{
	Mesh mesh;

	tinyobj::ObjReaderConfig conf;
	conf.mtl_search_path = "./";

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(path.data(), conf))
	{
		const auto& error = reader.Error();

		if (!error.empty())
			throw std::runtime_error(fmt::format("tinyobjloader: {}", error));
		else
			throw std::runtime_error("tinyobjloader: Unknown Error");
	}

	const auto& warning = reader.Warning();
	if (!warning.empty())
		fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold, "tinyobjloader: {}", warning);

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();

	std::unordered_map<Vertex, uint32_t> dedupeMap;

	for (const auto& shape : shapes)
	{
		size_t index_offset = 0;
		for (const auto& fv : shape.mesh.num_face_vertices)
		{
			for (size_t v = 0; v < fv; v++)
			{
				Vertex vtx{ .color = { 1.0f, 1.0f, 1.0f } };
				auto& ix = shape.mesh.indices[index_offset + v];
				vtx.pos.x = attrib.vertices[3 * ix.vertex_index + 0];
				vtx.pos.y = attrib.vertices[3 * ix.vertex_index + 1];
				vtx.pos.z = attrib.vertices[3 * ix.vertex_index + 2];

				if (ix.normal_index >= 0)
				{
					vtx.normal.x = attrib.normals[3 * ix.normal_index + 0];
					vtx.normal.y = attrib.normals[3 * ix.normal_index + 1];
					vtx.normal.z = attrib.normals[3 * ix.normal_index + 2];
				}

				if (dedupeMap.count(vtx) == 0) {
					dedupeMap[vtx] = static_cast<uint32_t>(mesh.m_vertices.size());
					mesh.m_vertices.emplace_back(vtx);
				}

				mesh.m_indices.push_back(dedupeMap[vtx]);
			}

			index_offset += fv;
		}
	}

	return mesh;
}

Mesh::Mesh()
	: m_position(0.0f, 0.0f, 0.0f),
	m_rotProxy(0.0f, 0.0f, 0.0f),
	m_scale(1.0f, 1.0f, 1.0f)
{
	rotate(0.0f, { 1.0f, 1.0f, 1.0f });
}

void Mesh::translate(const glm::vec3& translation)
{
	m_position += translation;
}

void Mesh::translate(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Mesh::rotate(float angle, const glm::vec3& axis)
{
	m_orientation = glm::angleAxis(angle, axis);
}

void Mesh::scale(const glm::vec3& scale)
{
	m_scale += scale;
}

void Mesh::scale(float x, float y, float z)
{
	m_scale.x += x;
	m_scale.y += y;
	m_scale.z += z;
}

glm::mat4 Mesh::getModel() const
{
	glm::mat4 translationM = glm::translate(glm::identity<glm::mat4>(), m_position);
	glm::mat4 rotationM = glm::mat4_cast(m_orientation);
	glm::mat4 scaleM = glm::scale(glm::identity<glm::mat4>(), m_scale);

	return translationM * rotationM * scaleM;
}

void Mesh::renderImGuiMenu()
{
	ImGui::BeginChild(
		"mesh_controls", 
		ImVec2(0, 0), 
		ImGuiChildFlags_AutoResizeX | 
		ImGuiChildFlags_AutoResizeY |
		ImGuiChildFlags_AlwaysAutoResize
	);

	ImGui::Text("Mesh");
	ImGui::Separator();
	ImGui::DragFloat3("Position", reinterpret_cast<float*>(&m_position.x));
	
	if (ImGui::DragFloat3("Rotation", reinterpret_cast<float*>(&m_rotProxy.x), 1.0f, -180.0f, 180.0f))
		m_orientation = glm::quat(glm::radians(m_rotProxy));

	ImGui::DragFloat3("Scale", reinterpret_cast<float*>(&m_scale.x), 1.0f, 0.001f, INFINITY);
	ImGui::EndChild();
}

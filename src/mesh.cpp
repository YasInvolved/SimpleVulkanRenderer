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
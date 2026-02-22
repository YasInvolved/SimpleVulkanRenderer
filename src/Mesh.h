# pragma once

namespace svr
{
	class Mesh
	{
	public:
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 color;

			static constexpr VkVertexInputBindingDescription getBindingDescription()
			{
				constexpr VkVertexInputBindingDescription desc =
				{
					.binding = 0,
					.stride = sizeof(Vertex),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
				};

				return desc;
			}

			using VertAttrDesc_t = std::array<VkVertexInputAttributeDescription, 2>;
			static constexpr VertAttrDesc_t getAttributeDesc()
			{
				constexpr VertAttrDesc_t desc =
				{
					{
						{
							.location = 0,
							.binding = 0,
							.format = VK_FORMAT_R32G32B32_SFLOAT,
							.offset = offsetof(Vertex, pos),
						},
						{
							.location = 1,
							.binding = 0,
							.format = VK_FORMAT_R32G32B32_SFLOAT,
							.offset = offsetof(Vertex, color),
						}
					}
				};

				return desc;
			}

			bool operator==(const Vertex&) const;
		};

		using vertices_t = std::vector<Vertex>;
		using indices_t = std::vector<uint32_t>;

	private:
		vertices_t m_vertices;
		indices_t m_indices;

	public:
		static Mesh loadObjMesh(const std::string_view path);

		const vertices_t& getVertices() const { return m_vertices; }
		const indices_t& getIndices() const { return m_indices; }

		glm::mat4 getModel() const { return glm::identity<glm::mat4>(); } // TODO: implement transformations
	};
}
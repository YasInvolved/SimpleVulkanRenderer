# pragma once

namespace svr
{
	class Mesh
	{
	public:
		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
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

			using VertAttrDesc_t = std::array<VkVertexInputAttributeDescription, 3>;
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
							.offset = offsetof(Vertex, normal),
						},
						{
							.location = 2,
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

		glm::vec3 m_position;
		glm::quat m_orientation;
		glm::vec3 m_rotProxy;
		glm::vec3 m_scale;

	public:
		Mesh();
		
		using Index_t = uint32_t;

		static Mesh loadObjMesh(const std::string_view path);

		const vertices_t& getVertices() const { return m_vertices; }
		const indices_t& getIndices() const { return m_indices; }
		
		void translate(const glm::vec3& translation);
		void translate(float x, float y, float z);

		void rotate(float angle, const glm::vec3& axis);

		void scale(const glm::vec3& scale);
		void scale(float x, float y, float z);

		glm::mat4 getModel() const;

		void renderImGuiMenu();
	};
}

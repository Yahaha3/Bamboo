#pragma once

#include "runtime/resource/asset/base/mesh.h"
#include "runtime/resource/asset/base/asset.h"

namespace Bamboo
{
	class SkeletalMesh : public Mesh, public Asset
	{
	public:
		SkeletalMesh(const URL& url);
		virtual void inflate() override;

		std::vector<SkeletalVertex> m_vertices;

	private:
		friend class cereal::access;
		template<class Archive>
		void archive(Archive& ar) const
		{
			ar(cereal::base_class<Mesh>(this));
			ar(m_vertices.size());
		}

		template<class Archive>
		void save(Archive& ar) const
		{
			archive(ar);
		}

		template<class Archive>
		void load(Archive& ar)
		{
			archive(ar);

			inflate();
		}
	};
}
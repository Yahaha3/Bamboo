#pragma once

#include "runtime/function/framework/entity/entity.h"
#include "runtime/resource/asset/base/asset.h"
#include <unordered_map>

namespace Bamboo
{
	class World : public Asset
	{
	public:
		void tick(float delta_time);

		const auto& getEntities() const { return m_entites; }
		std::shared_ptr<Entity> getEntity(EntityID id);
		bool removeEntity(EntityID id);

	private:
		std::unordered_map<EntityID, std::shared_ptr<Entity>> m_entites;
	};
}
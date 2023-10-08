#include "world_manager.h"
#include "engine/core/base/macro.h"
#include "engine/core/config/config_manager.h"
#include "engine/resource/asset/asset_manager.h"
#include "engine/resource/asset/skeletal_mesh.h"
#include "engine/resource/asset/texture_2d.h"
#include "engine/resource/asset/texture_cube.h"
#include "engine/resource/asset/animation.h"
#include "engine/function/framework/component/transform_component.h"
#include "engine/function/framework/component/camera_component.h"
#include "engine/function/framework/component/directional_light_component.h"
#include "engine/function/framework/component/sky_light_component.h"
#include "engine/function/framework/component/spot_light_component.h"

namespace Bamboo
{

	void WorldManager::init()
	{
		URL default_world_url = g_engine.configManager()->getDefaultWorldUrl();

		loadWorld(default_world_url);
		//scriptWorld();
	}

	void WorldManager::destroy()
	{
		m_current_world.reset();
	}

	void WorldManager::tick(float delta_time)
	{
		// open world async
		if (!m_open_world_url.empty())
		{
			loadWorld(m_open_world_url);
			m_open_world_url.clear();
		}

		// create world async
		if (!m_template_url.empty())
		{
			loadWorld(m_template_url);
			saveAsWorld(m_save_as_url);
			loadWorld(m_save_as_url);

			m_template_url.clear();
			m_save_as_url.clear();
		}

		m_current_world->tick(delta_time);
	}

	void WorldManager::openWorld(const URL& url)
	{
		m_open_world_url = url;
		LOG_INFO("open world: {}", m_open_world_url);
	}

	void WorldManager::createWorld(const URL& template_url, const URL& save_as_url)
	{
		m_template_url = template_url;
		m_save_as_url = save_as_url;
		LOG_INFO("create world: {}", m_template_url);
	}

	bool WorldManager::saveWorld()
	{
		g_engine.assetManager()->serializeAsset(m_current_world);
		LOG_INFO("save world: {}", m_current_world->getURL());
		return true;
	}

	bool WorldManager::saveAsWorld(const URL& url)
	{
		g_engine.assetManager()->serializeAsset(m_current_world, url);
		return true;
	}

	std::string WorldManager::getCurrentWorldName()
	{
		return g_engine.fileSystem()->basename(m_current_world->getURL());
	}

	std::weak_ptr<CameraComponent> WorldManager::getCameraComponent()
	{
		return m_current_world->getCameraEntity().lock()->getComponent(CameraComponent);
	}

	bool WorldManager::loadWorld(const URL& url)
	{
		if (m_current_world)
		{
			m_current_world.reset();
		}

		m_current_world = g_engine.assetManager()->loadAsset<World>(url);
		return true;
	}

	void WorldManager::scriptWorld()
	{
 		// add directional light
 		std::shared_ptr<TransformComponent> transform_component = std::make_shared<TransformComponent>();
 		transform_component->m_position = glm::vec3(0.0f, 5.0f, 0.0f);
 		transform_component->m_rotation = glm::vec3(0.0f, -150.0f, -20.0f);
 
 		std::shared_ptr<Entity> directional_light_entity = m_current_world->createEntity("directional_light");
 		directional_light_entity->addComponent(transform_component);
 		directional_light_entity->addComponent(std::make_shared<DirectionalLightComponent>());
 
		m_current_world->removeEntity(1);
	}

}
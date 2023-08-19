#include "point_light_component.h"

RTTR_REGISTRATION
{
rttr::registration::class_<Bamboo::PointLightComponent>("PointLightComponent")
	 .property("radius", &Bamboo::PointLightComponent::m_radius)
	 .property("attenuation", &Bamboo::PointLightComponent::m_attenuation);
}

CEREAL_REGISTER_TYPE(Bamboo::PointLightComponent)
CEREAL_REGISTER_POLYMORPHIC_RELATION(Bamboo::LightComponent, Bamboo::PointLightComponent)

namespace Bamboo
{
	POLYMORPHIC_DEFINITION(PointLightComponent)
}
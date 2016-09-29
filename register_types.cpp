
/* register_types.cpp */

#include "register_types.h"
#include "object_type_db.h"
#include "vegetationInstance.h"

void register_vegetationInstance_types() {

        ObjectTypeDB::register_type<VegetationInstance>();
}

void unregister_vegetationInstance_types() {

}

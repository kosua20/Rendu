#include "scene/Object.hpp"


Object::Object() {}

Object::Object(const Object::Type type, const MeshInfos * mesh, bool castShadows){
	_material = type;
	_castShadow = castShadows;
	_mesh = mesh;
}

void Object::decode(const std::vector<KeyValues> & params){
	
	// \todo Find a way to easily expand this.
	const std::map<std::string, Object::Type> types = {{"PBRRegular", Type::PBRRegular},
		{"PBRParallax", Type::PBRParallax}, {"Skybox", Type::Skybox}, {"Common", Type::Common}};
	
	
	// We know there is only one transformation in the parameters set.
	_model = Codable::decodeTransformation(params);
	
	for(int pid = 0; pid < params.size();){
		const auto & param = params[pid];
		if(param.key == "type" && !param.values.empty()){
			const std::string typeString = param.values[0];
			if(types.count(typeString) > 0){
				_material = types.at(typeString);
			}
			
		} else if(param.key == "mesh" && !param.values.empty()){
			const std::string meshString = param.values[0];
			_mesh = Resources::manager().getMesh(meshString);
			
		} else if(param.key == "shadows" && !param.values.empty()){
			const std::string shadowString = param.values[0];
			_castShadow = (shadowString == "true");
			
		} else if(param.key == "textures"){
			// Move to the next parameter.
			++pid;
			while(pid < params.size()){
				const TextureInfos * tex = Codable::decodeTexture(params[pid]);
				if(tex == nullptr){
					// In this case, decrement pid again, this is something else.
					--pid;
					break;
				}
				addTexture(tex);
				++pid;
			}
		} else if(param.key == "animations"){
			// Move to the next parameter.
			++pid;
			while(pid < params.size()){
				const std::shared_ptr<Animation> anim = Animation::decode(params[pid]);
				if(anim == nullptr) {
					// In this case, decrement pid again, this is something else.
					--pid;
					break;
				}
				addAnimation(anim);
				++pid;
			}
		}
		++pid;
	}
	
}

void Object::addTexture(const TextureInfos * infos){
	_textures.push_back(infos);
}

void Object::addAnimation(std::shared_ptr<Animation> anim){
	_animations.push_back(anim);
}

void Object::update(double fullTime, double frameTime) {
	glm::mat4 model = _model;
	for(auto anim : _animations){
		model = anim->apply(model, fullTime, frameTime);
	}
	_model = model;
}

BoundingBox Object::getBoundingBox() const {
	return _mesh->bbox.transformed(_model);
}



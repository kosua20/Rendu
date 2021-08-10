#include "graphics/ShaderCompiler.hpp"
#include "graphics/GPU.hpp"
#include "graphics/GPUInternal.hpp"

#include "system/TextUtilities.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <map>
#include <sstream>

// SPIRV compilation settings based on glslang standalone example
static const TBuiltInResource defaultBuiltInResources = {
	/* .MaxLights = */ 32, /* .MaxClipPlanes = */ 6, /* .MaxTextureUnits = */ 32,
	/* .MaxTextureCoords = */ 32,
	/* .MaxVertexAttribs = */ 64,
	/* .MaxVertexUniformComponents = */ 4096,
	/* .MaxVaryingFloats = */ 64,
	/* .MaxVertexTextureImageUnits = */ 32,
	/* .MaxCombinedTextureImageUnits = */ 80,
	/* .MaxTextureImageUnits = */ 32,
	/* .MaxFragmentUniformComponents = */ 4096,
	/* .MaxDrawBuffers = */ 32,
	/* .MaxVertexUniformVectors = */ 128,
	/* .MaxVaryingVectors = */ 8,
	/* .MaxFragmentUniformVectors = */ 16,
	/* .MaxVertexOutputVectors = */ 16,
	/* .MaxFragmentInputVectors = */ 15,
	/* .MinProgramTexelOffset = */ -8,
	/* .MaxProgramTexelOffset = */ 7,
	/* .MaxClipDistances = */ 8,
	/* .MaxComputeWorkGroupCountX = */ 65535,
	/* .MaxComputeWorkGroupCountY = */ 65535,
	/* .MaxComputeWorkGroupCountZ = */ 65535,
	/* .MaxComputeWorkGroupSizeX = */ 1024,
	/* .MaxComputeWorkGroupSizeY = */ 1024,
	/* .MaxComputeWorkGroupSizeZ = */ 64,
	/* .MaxComputeUniformComponents = */ 1024,
	/* .MaxComputeTextureImageUnits = */ 16,
	/* .MaxComputeImageUniforms = */ 8,
	/* .MaxComputeAtomicCounters = */ 8,
	/* .MaxComputeAtomicCounterBuffers = */ 1,
	/* .MaxVaryingComponents = */ 60,
	/* .MaxVertexOutputComponents = */ 64,
	/* .MaxGeometryInputComponents = */ 64,
	/* .MaxGeometryOutputComponents = */ 128,
	/* .MaxFragmentInputComponents = */ 128,
	/* .MaxImageUnits = */ 8,
	/* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
	/* .MaxCombinedShaderOutputResources = */ 8,
	/* .MaxImageSamples = */ 0,
	/* .MaxVertexImageUniforms = */ 0,
	/* .MaxTessControlImageUniforms = */ 0,
	/* .MaxTessEvaluationImageUniforms = */ 0,
	/* .MaxGeometryImageUniforms = */ 0,
	/* .MaxFragmentImageUniforms = */ 8,
	/* .MaxCombinedImageUniforms = */ 8,
	/* .MaxGeometryTextureImageUnits = */ 16,
	/* .MaxGeometryOutputVertices = */ 256,
	/* .MaxGeometryTotalOutputComponents = */ 1024,
	/* .MaxGeometryUniformComponents = */ 1024,
	/* .MaxGeometryVaryingComponents = */ 64,
	/* .MaxTessControlInputComponents = */ 128,
	/* .MaxTessControlOutputComponents = */ 128,
	/* .MaxTessControlTextureImageUnits = */ 16,
	/* .MaxTessControlUniformComponents = */ 1024,
	/* .MaxTessControlTotalOutputComponents = */ 4096,
	/* .MaxTessEvaluationInputComponents = */ 128,
	/* .MaxTessEvaluationOutputComponents = */ 128,
	/* .MaxTessEvaluationTextureImageUnits = */ 16,
	/* .MaxTessEvaluationUniformComponents = */ 1024,
	/* .MaxTessPatchComponents = */ 120,
	/* .MaxPatchVertices = */ 32,
	/* .MaxTessGenLevel = */ 64,
	/* .MaxViewports = */ 16,
	/* .MaxVertexAtomicCounters = */ 0,
	/* .MaxTessControlAtomicCounters = */ 0,
	/* .MaxTessEvaluationAtomicCounters = */ 0,
	/* .MaxGeometryAtomicCounters = */ 0,
	/* .MaxFragmentAtomicCounters = */ 8,
	/* .MaxCombinedAtomicCounters = */ 8,
	/* .MaxAtomicCounterBindings = */ 1,
	/* .MaxVertexAtomicCounterBuffers = */ 0,
	/* .MaxTessControlAtomicCounterBuffers = */ 0,
	/* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
	/* .MaxGeometryAtomicCounterBuffers = */ 0,
	/* .MaxFragmentAtomicCounterBuffers = */ 1,
	/* .MaxCombinedAtomicCounterBuffers = */ 1,
	/* .MaxAtomicCounterBufferSize = */ 16384,
	/* .MaxTransformFeedbackBuffers = */ 4,
	/* .MaxTransformFeedbackInterleavedComponents = */ 64,
	/* .MaxCullDistances = */ 8,
	/* .MaxCombinedClipAndCullDistances = */ 8,
	/* .MaxSamples = */ 4,
	/* .maxMeshOutputVerticesNV = */ 256,
	/* .maxMeshOutputPrimitivesNV = */ 512,
	/* .maxMeshWorkGroupSizeX_NV = */ 32,
	/* .maxMeshWorkGroupSizeY_NV = */ 1,
	/* .maxMeshWorkGroupSizeZ_NV = */ 1,
	/* .maxTaskWorkGroupSizeX_NV = */ 32,
	/* .maxTaskWorkGroupSizeY_NV = */ 1,
	/* .maxTaskWorkGroupSizeZ_NV = */ 1,
	/* .maxMeshViewCountNV = */ 4,
	/* .maxDualSourceDrawBuffersEXT = */ 1,

	/* .limits = */ {
		/* .nonInductiveForLoops = */ 1,
		/* .whileLoops = */ 1,
		/* .doWhileLoops = */ 1,
		/* .generalUniformIndexing = */ 1,
		/* .generalAttributeMatrixVectorIndexing = */ 1,
		/* .generalVaryingIndexing = */ 1,
		/* .generalSamplerIndexing = */ 1,
		/* .generalVariableIndexing = */ 1,
		/* .generalConstantMatrixVectorIndexing = */ 1,
	}};

bool ShaderCompiler::init(){
	return glslang::InitializeProcess();
}

void ShaderCompiler::cleanup(){
	glslang::FinalizeProcess();
}

void ShaderCompiler::clean(Program::Stage & stage){
	GPUContext* context = GPU::getInternal();
	vkDestroyShaderModule(context->device, stage.module, nullptr);
	stage.reset();
}

void ShaderCompiler::compile(const std::string & prog, ShaderType type, Program::Stage & stage, std::string & finalLog) {

	// Add GLSL version.
	std::string outputProg = "#version 450\n#extension GL_ARB_separate_shader_objects : enable\n#line 1 0\n";
	outputProg.append(prog);

	// Create shader object.
	static const std::unordered_map<ShaderType, EShLanguage> types = {
		{ShaderType::VERTEX, EShLangVertex},
		{ShaderType::FRAGMENT, EShLangFragment},
		{ShaderType::GEOMETRY, EShLangGeometry},
		{ShaderType::TESSCONTROL, EShLangTessControl},
		{ShaderType::TESSEVAL, EShLangTessEvaluation}
	};
	const char* progStr = outputProg.c_str();
	const EShLanguage stageDest = types.at(type);
	glslang::TShader shader(stageDest);
	shader.setStrings(&progStr, 1);
	shader.setEntryPoint("main");
	shader.setEnvInput(glslang::EShSourceGlsl, stageDest, glslang::EShClientVulkan, 100);
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
	shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);


	finalLog = "";
	const EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules);
	bool success = shader.parse(&defaultBuiltInResources, 110, true, messages);
	if(!success){
		std::string infoLogString(shader.getInfoLog());
		TextUtilities::replace(infoLogString, "\n", "\n\t");
		infoLogString.insert(0, "\t");
		finalLog = infoLogString;
		return;
	}

	glslang::TProgram program;
	program.addShader(&shader);
	success = program.link(messages);
	if(!success){
		std::string infoLogString(program.getInfoLog());
		finalLog = infoLogString;
		return;
	}
	if(!program.mapIO()){
		finalLog = "Unable to map IO.";
		return;
	}

	std::vector<unsigned int> spirv;
	glslang::SpvOptions spvOptions;
	spvOptions.generateDebugInfo = false;
	spvOptions.disableOptimizer = false;
	spvOptions.optimizeSize = true;
	spvOptions.disassemble = false;
	spvOptions.validate = false;
	glslang::GlslangToSpv(*program.getIntermediate(stageDest), spirv, &spvOptions);
	if(spirv.empty()){
		finalLog = "Unable to generate SPIRV.";
		return;
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(unsigned int);
	createInfo.pCode = reinterpret_cast<const uint32_t*>(spirv.data());
	VkShaderModule shaderModule;
	GPUContext* context = GPU::getInternal();
	if(vkCreateShaderModule(context->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		finalLog = "Unable to create shader module.";
		return;
	}

	stage.module = shaderModule;

	reflect(program, stage);
}

Program::UniformDef::Type ShaderCompiler::convertType(const glslang::TType& type){

	using Type = Program::UniformDef::Type;

	const glslang::TBasicType baseType = type.getBasicType();

	// Build a supported type matrix.
	static const std::map<glslang::TBasicType, std::array<Type, 5>> typesMatrix = {
		{glslang::EbtBool, {Type::BOOL, Type::BOOL, Type::BVEC2, Type::BVEC3, Type::BVEC4}},
		{glslang::EbtInt, {Type::INT, Type::INT, Type::IVEC2, Type::IVEC3, Type::IVEC4}},
		{glslang::EbtUint, {Type::UINT, Type::UINT, Type::UVEC2, Type::UVEC3, Type::UVEC4}},
		{glslang::EbtFloat, {Type::FLOAT, Type::FLOAT, Type::VEC2, Type::VEC3, Type::VEC4}},
	};

	static const std::array<Type, 5> matricesList = {
		Type::FLOAT, Type::FLOAT, Type::MAT2, Type::MAT3, Type::MAT4
	};

	// Skip unsuported basic types.
	if(typesMatrix.find(baseType) == typesMatrix.end()){
		return Type::OTHER;
	}

	if(type.isScalar() || type.isVector()){
		const int size = type.getVectorSize();
		if(size > 4){
			return Type::OTHER;
		}
		return typesMatrix.at(baseType)[size];
	}

	if(type.isMatrix()){
		const int rows = type.getMatrixRows();
		const int cols = type.getMatrixCols();

		if(baseType != glslang::EbtFloat || (rows != cols) || (rows > 4) || (rows < 1)){
			return Type::OTHER;
		}
		return matricesList[rows];
	}
	return Program::UniformDef::Type::OTHER;
}

uint ShaderCompiler::getSetFromType(const glslang::TType& type){
	const glslang::TQualifier& qualifier = type.getQualifier();
	return qualifier.hasSet() ? (qualifier.layoutSet & qualifier.layoutSetEnd) : 0u;
}

void ShaderCompiler::reflect(glslang::TProgram & program, Program::Stage & stage){

	program.buildReflection(EShReflectionStrictArraySuffix | EShReflectionBasicArraySuffix);

	// Retrieve UBOs infos.
	const size_t uboCount = program.getNumUniformBlocks();
	stage.buffers.resize(uboCount);

	for(size_t uid = 0; uid < uboCount; ++uid){
		const glslang::TObjectReflection& ubo = program.getUniformBlock(uid);
		Program::BufferDef& def = stage.buffers[ubo.index];
		def.binding = ubo.getBinding();
		def.name = ubo.name;
		def.size = ubo.size;
		// Retrieve set index stored on type.
		def.set = getSetFromType(*ubo.getType());

	}

	static const std::map<glslang::TSamplerDim, TextureShape> texShapes = {
		{glslang::TSamplerDim::Esd1D, TextureShape::D1},
		{glslang::TSamplerDim::Esd2D, TextureShape::D2},
		{glslang::TSamplerDim::Esd3D, TextureShape::D3},
		{glslang::TSamplerDim::EsdCube, TextureShape::Cube}
	};

	// Retrieve each uniform infos.
	const size_t uniformCount = program.getNumUniformVariables();
	for(size_t uid = 0; uid < uniformCount; ++uid){
		const glslang::TObjectReflection& uniform = program.getUniform(uid);
		const glslang::TType& type = *uniform.getType();

		const int binding = uniform.getBinding();
		// If the variable is freely bound, it's a texture.
		if(binding >= 0){
			// This is a sampler.
			const glslang::TSampler& sampler = uniform.getType()->getSampler();
			if(texShapes.count(sampler.dim) == 0){
				Log::Error() << "Unsupported texture shape in shader." << std::endl;
				continue;
			}

			stage.samplers.emplace_back();
			Program::SamplerDef& def = stage.samplers.back();
			def.name = uniform.name;
			def.binding = binding;
			def.set = getSetFromType(type);
			def.shape = texShapes.at(sampler.dim);
			if(sampler.isArrayed()){
				def.shape = def.shape | TextureShape::Array;
			}
			continue;
		}
		
		// Else, uniform in buffer
		Program::BufferDef& containingBuffer = stage.buffers[uniform.index];

		// Arrays containing basic types are not expanded automatically.
		if(type.isArray()){
			if(type.isUnsizedArray() || type.getArraySizes()->getNumDims() > 1){
				Log::Warning() << Log::GPU << "Unsupported unsized/multi-level array in shader." << std::endl;
				continue;
			}
			const uint size = static_cast<uint>(uniform.getType()->getArraySizes()->getDimSize(0));
			const std::string::size_type lastOpeningBracket = uniform.name.find_last_of('[');
			const std::string finalName = uniform.name.substr(0, lastOpeningBracket);
			// We need a non-array version of the type.
			glslang::TType* typeWithoutArray = type.clone();
			typeWithoutArray->clearArraySizes();
			const Program::UniformDef::Type elemType = convertType(*typeWithoutArray);
			// This works because this only happens for basic types.
			// Minimal alignment is 16 bytes.
			const uint elemOffset = std::max(typeWithoutArray->computeNumComponents() * 4, 16);
			delete typeWithoutArray;

			for(uint iid = 0; iid < size; ++iid){
				containingBuffer.members.emplace_back();
				Program::UniformDef& def = containingBuffer.members.back();
				def.name = finalName + "[" + std::to_string(iid) + "]";
				def.type = elemType;
				Program::UniformDef::Location location;
				location.binding = containingBuffer.binding;
				location.offset = uniform.offset + iid * elemOffset;
				def.locations.emplace_back(location);
			}

		} else {
			containingBuffer.members.emplace_back();
			Program::UniformDef& def = containingBuffer.members.back();
			def.name = uniform.name;
			def.type = convertType(type);
			Program::UniformDef::Location location;
			location.binding = containingBuffer.binding;
			location.offset = uniform.offset;
			def.locations.emplace_back(location);
		}

	}

}

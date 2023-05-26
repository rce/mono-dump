#include "mono.h"

namespace mono {
	const char* ModuleName = "mono-2.0-bdwgc.dll";


#define WRAPPER(func) MonoFunction<decltype(&::mono_##func)> func("mono_"#func)

	WRAPPER(get_root_domain);

	// threads.h
	WRAPPER(thread_attach);
	WRAPPER(thread_detach);

	// image.h
	WRAPPER(image_get_name);
	WRAPPER(image_get_table_info);
	WRAPPER(table_info_get_rows);

	// assembly.h
	WRAPPER(assembly_foreach);
	WRAPPER(assembly_get_image);

	// class.h
	WRAPPER(class_get);
	WRAPPER(class_get_name);
	WRAPPER(class_get_namespace);
	WRAPPER(class_get_fields);

	WRAPPER(field_get_type);
	WRAPPER(type_get_type);
	//WRAPPER(field_get_parent);
	WRAPPER(field_get_offset);
	WRAPPER(field_get_flags);
	WRAPPER(field_get_name);
	WRAPPER(type_get_name);
	WRAPPER(type_size);
	WRAPPER(class_vtable);


	ThreadAttachment::ThreadAttachment() {
		auto domain = mono::get_root_domain();
		this->monohread = mono::thread_attach(domain);
	}

	ThreadAttachment::~ThreadAttachment() {
		if (this->monohread != nullptr) {
			mono::thread_detach(this->monohread);
			this->monohread = nullptr;
		}
	}

	std::vector<MonoAssembly*> get_assemblies() {
		std::vector<MonoAssembly*> assemblies;
		mono::assembly_foreach([](void* assembly, void* user_data) {
			auto vec = static_cast<std::vector<MonoAssembly*>*>(user_data);
			vec->push_back(static_cast<MonoAssembly*>(assembly));
			}, &assemblies);
		return assemblies;
	}
}
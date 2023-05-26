#pragma once

#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/tokentype.h>

#include <iostream>
#include <type_traits>
#include <optional>
#include <vector>
#include <Windows.h>

namespace mono {
	extern const char* ModuleName;

	template <typename Function>
	class MonoFunction {
		const std::string name;
		Function ptr;
	public:
		MonoFunction(const std::string& name) : name(name) {}
		Function Get() {
			if (ptr == nullptr) {
				const HMODULE h = GetModuleHandle(ModuleName);
				this->ptr = reinterpret_cast<Function>(GetProcAddress(h, name.c_str()));
				std::cerr << name << ": " << std::hex << this->ptr << std::dec << std::endl;
			}
			return ptr;
		}

		template <typename... Args>
		decltype(auto) operator()(Args&& ...args) {
			return this->Get()(std::forward<Args>(args)...);
		}
	};
	
#define WRAPPER(func) extern MonoFunction<decltype(&::mono_##func)> func

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

	class ThreadAttachment {
		MonoThread* monohread = nullptr;
	public:
		ThreadAttachment();
		~ThreadAttachment();
	};
	std::vector<MonoAssembly*> get_assemblies();
}
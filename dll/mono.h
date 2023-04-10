#pragma once

#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/tokentype.h>

namespace mono {
	template <typename Function>
	class MonoFunction {
		const std::string name;
		Function ptr;
	public:
		MonoFunction(const std::string& name) : name(name) {}
		Function Get() {
			if (ptr == nullptr) {
				const HMODULE h = GetModuleHandle("mono-2.0-bdwgc.dll");
				this->ptr = reinterpret_cast<Function>(GetProcAddress(h, name.c_str()));
				std::cerr << name << ": " << std::hex << this->ptr << std::dec << std::endl;
				CloseHandle(h);
			}
			return ptr;
		}
	};

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
}
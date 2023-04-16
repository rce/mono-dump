#pragma once

#include <mono/metadata/threads.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/class.h>
#include <mono/metadata/tokentype.h>

#include <iostream>

namespace mono {
	auto ModuleName = "mono-2.0-bdwgc.dll";
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
	WRAPPER(class_get_name);
	WRAPPER(class_get_namespace);
	WRAPPER(class_get_fields);

	WRAPPER(field_get_type);
	WRAPPER(type_get_type);
	WRAPPER(field_get_offset);
	WRAPPER(field_get_name);
	WRAPPER(type_get_name);



	class ThreadAttachment {
		MonoThread* monohread = nullptr;

	public:
		ThreadAttachment() {
			if (mono::get_root_domain.Get()) {
				std::cout << "mono_get_root_domain()" << std::endl;
				auto domain = mono::get_root_domain.Get()();
				if (mono::thread_attach.Get()) {
					std::cout << "mono_thread_attach()" << std::endl;
					this->monohread = mono::thread_attach.Get()(domain);
				}
			}
		}

		~ThreadAttachment() {
			if (this->monohread != nullptr) {
				if (mono::thread_detach.Get()) {
					std::cout << "mono_thread_detach()" << std::endl;
					mono::thread_detach.Get()(this->monohread);
					this->monohread = nullptr;
				}
			}
		}
	};
}
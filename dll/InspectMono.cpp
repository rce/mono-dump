#include "InspectMono.h"

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <locale>
#include <codecvt>

#include "mono.h"

#define PRINT_HEX(x) std::cout << #x << " " << std::hex << (x) << std::dec << std::endl
#define LOG(x) std::cerr << #x << " == " << x << std::endl

template <typename T>
class HandleCloser {
	T t;
	typedef void (*Closer)(T t);
	Closer closer;
public:
	HandleCloser(T t, Closer closer) : t(t), closer(closer) {}
	~HandleCloser() {
		closer(this->t);
	}
	T Get() { return this->t; }
};

void _cdecl push_assembly_to_vector(MonoAssembly* assembly, std::vector<MonoAssembly*>* assemblies)
{
	assemblies->push_back(assembly);
}

void InspectMono() {
	// TODO Sometimes the module is mono.dll, sometimes mono-2.0-bdwgc. One could use something like
	// CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId()) and GetProccAddress("mono_thread_attach")
	// or something to figure out the proper module.
	HandleCloser<HMODULE> h(
		GetModuleHandle("mono-2.0-bdwgc.dll"),
		[](HMODULE h) { CloseHandle(h); }
	);

	if (!h.Get()) {
		std::cout << "Mono DLL not found" << std::endl;
		return;
	}

	if (!GetProcAddress(h.Get(), "mono_thread_attach")) {
		std::cout << "mono_thread_attach not found" << std::endl;
		return;
	}

	std::cout << "Mono DLL location: " << std::hex << h.Get() << std::dec << std::endl;
	if (GetProcAddress(h.Get(), "il2cpp_thread_attach")) {
		std::cout << "Using il2cpp, no can do" << std::endl;
		return;
	}

	MonoDomain* domain = mono::get_root_domain.Get()();
	MonoThread* monothread = nullptr;
	if (mono::thread_attach.Get()) {
		monothread = mono::thread_attach.Get()(domain);
	}

	if (monothread != nullptr) {
		std::cout << "detaching monothread" << std::endl;
		if (mono::thread_detach.Get()) {
			std::cerr << "mono_thread_detach(...)" << std::endl;
			mono::thread_detach.Get()(monothread);
		}
		else {
			std::cerr << "mono_thread_detach is not available" << std::endl;
		}
	}


	std::vector<MonoAssembly*> assemblies;
	mono::assembly_foreach.Get()((MonoFunc)push_assembly_to_vector, &assemblies);
	for (auto& assembly : assemblies) {
		auto image = mono::assembly_get_image.Get()(assembly);
		auto image_name = std::string(mono::image_get_name.Get()(image));
		LOG(image_name);

		auto tdef = mono::image_get_table_info.Get()(image, MONO_TABLE_TYPEDEF);
		if (!tdef) continue;

		auto tdefcount = mono::table_info_get_rows.Get()(tdef);
		LOG(tdefcount);
		for (int i = 0; i < tdefcount; i++)
		{
			// This crashes :(
			// auto c = mono::class_get.Get()(image, MONO_TOKEN_TYPE_DEF | (i + 1));
		}
	}
};

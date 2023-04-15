#include "InspectMono.h"

#include <Windows.h>
#include <iostream>
#include <vector>
#include <string.h>
#include <locale>
#include <codecvt>
#include <fstream>

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

void DumpMono() {

	MonoDomain* domain = mono::get_root_domain.Get()();
	MonoThread* monothread = nullptr;
	if (mono::thread_attach.Get()) {
		monothread = mono::thread_attach.Get()(domain);
	}


	std::ofstream output("monodump.txt");

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
			auto klass = mono::class_get.Get()(image, MONO_TOKEN_TYPE_DEF | (i + 1));
			auto name = mono::class_get_name.Get()(klass);
			auto ns = mono::class_get_namespace.Get()(klass);
			std::cout << ns << "." << name << "\n";
			output << "// " << ns << "." << name << "\n";
			output << "class " << name << " {\n";

			struct Field {
				std::string type_name;
				std::string field_name;
				uint32_t offset;
			};
			std::vector<Field> fields;
			void* iter = nullptr;
			MonoClassField* field = nullptr;
			do {
				field = mono::class_get_fields.Get()(klass, &iter);
				if (field) {
					auto field_name = mono::field_get_name.Get()(field);
					auto field_type = mono::field_get_type.Get()(field);
					auto field_type_name = mono::type_get_name.Get()(field_type);
					auto field_offset = mono::field_get_offset.Get()(field);
					fields.push_back(Field{ field_type_name, field_name, field_offset });
				}
			} while (field);
			std::sort(fields.begin(), fields.end(), [](Field a, Field b) { return a.offset < b.offset; });
			for (auto& f : fields) {
				output << "  " << f.type_name << " " << f.field_name << "; // Offset: 0x" << std::hex << f.offset << "\n";
			}
			output << "};\n\n";
		}
	}

	if (monothread && mono::thread_detach.Get()) {
		mono::thread_detach.Get()(monothread);
		monothread = nullptr;
	}

	output.close();
};


void InspectMono() {
	// TODO Sometimes the module is mono.dll, sometimes mono-2.0-bdwgc. One could use something like
	// CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId()) and GetProccAddress("mono_thread_attach")
	// or something to figure out the proper module.
	HandleCloser<HMODULE> h(
		GetModuleHandle(mono::ModuleName),
		[](HMODULE h) { CloseHandle(h); }
	);

	if (!h.Get()) {
		std::cout << "Mono DLL not found" << std::endl;
		return;
	}

	if (!mono::thread_attach.Get()) {
		std::cout << "mono_thread_attach not found" << std::endl;
		return;
	}

	std::cout << "Mono DLL location: " << std::hex << h.Get() << std::dec << std::endl;
	if (GetProcAddress(h.Get(), "il2cpp_thread_attach")) {
		std::cout << "Using il2cpp, no can do" << std::endl;
		return;
	}
	DumpMono();
}

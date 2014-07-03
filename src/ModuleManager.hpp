#ifndef MODULEMANAGER_HPP_UWM78W
#define MODULEMANAGER_HPP_UWM78W

#include "Export.hpp"
#include "Module.hpp"

namespace autobind {

class ModuleManager
{
	std::unordered_map<std::string, Module> _modules;

public:
	Module &module(const std::string &name)
	{
		auto it = _modules.find(name);
		if(it == _modules.end())
		{
			auto &result = _modules[name];
			result.setName(name);
			return result;
		}
		else
		{
			return it->second;
		}
	}

	AB_RETURN_AUTO(moduleStream() const,
	               streams::stream(_modules));


	void codegen(std::ostream &out) const
	{
		for(const auto &item : _modules)
		{
			item.second.codegen(out);
		}
	}

};

} // autobind

#endif // MODULEMANAGER_HPP_UWM78W

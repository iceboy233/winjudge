#pragma once

#include "Fs.h"

namespace JudgeFs {

public ref class RamFs : public Fs {
public:
	RamFs();
	void Set(System::String ^name, System::String ^value);
};

}

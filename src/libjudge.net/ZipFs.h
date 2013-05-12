#pragma once

#include "Fs.h"

namespace JudgeFs {

public ref class ZipFs : public Fs {
public:
	ZipFs(System::String ^path);
};

}

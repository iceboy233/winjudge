#pragma once

#include <judge.h>

namespace Judge {

public ref class JudgeException : public System::Exception {
public:
	JudgeException(jstatus_t status);
	jstatus_t StatusCode();

private:
	System::String ^_to_message(jstatus_t status);

private:
	jstatus_t status_;
};

}

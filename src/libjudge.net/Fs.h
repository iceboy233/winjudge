#pragma once

#include <judgefs.h>
#include <msclr/marshal.h>

namespace JudgeFs {

ref class FsFindResult;
ref class FsStream;

public ref class Fs abstract {
public:
	Fs();
	virtual ~Fs() { this->!Fs(); }
	!Fs();
	struct judgefs *Handle();
	FsFindResult ^Find(System::String ^prefix);
	FsStream ^Open(System::String ^path);

protected:
	void SetHandle(struct judgefs *fs);

private:
	struct judgefs *fs_;
};

public ref class FsFindResult : public System::Collections::IEnumerable {
public:
	ref class Enumerator : public System::Collections::IEnumerator {
	public:
		Enumerator(Fs ^fs, System::String ^prefix);
		~Enumerator() { this->!Enumerator(); }
		!Enumerator();

		virtual void Reset();
		virtual bool MoveNext();

		virtual property System::Object ^Current {
			System::Object ^get();
		};
	private:
		msclr::interop::marshal_context mc_;
		struct judgefs *fs_;
		const char *prefix_;
		void *context_;
		bool end_;
		const char *current_;
	};

public:
	FsFindResult(Fs ^fs, System::String ^prefix);
	virtual System::Collections::IEnumerator ^GetEnumerator();

private:
	Fs ^fs_;
	System::String ^prefix_;
};

public ref class FsStream : public System::IO::Stream {
public:
	FsStream(Fs ^fs, System::String ^path);
	virtual ~FsStream() { this->!FsStream(); }
	!FsStream();
	
	virtual property bool CanRead {
		bool get() override { return true; }
	};

	virtual property bool CanSeek {
		bool get() override { return false; }
	};

	virtual property bool CanWrite {
		bool get() override { return false; }
	};

	virtual property long long Length {
		long long get() override { throw gcnew System::NotSupportedException(); }
	};

	virtual property long long Position {
		long long get() override { throw gcnew System::NotSupportedException(); }
		void set(long long) override { throw gcnew System::NotSupportedException(); }
	};

	virtual void Flush() override {}
	virtual long long Seek(long long, System::IO::SeekOrigin) override
		{ throw gcnew System::NotSupportedException(); }

	virtual void SetLength(long long) override { throw gcnew System::NotSupportedException(); }

	virtual int Read(array<unsigned char>^ buffer, int offset, int count) override;

	virtual void Write(array<unsigned char>^ buffer, int offset, int count) override
		{ throw gcnew System::NotSupportedException(); }

private:
	Fs ^fs_;
	struct judgefs_file *file_;
};

}

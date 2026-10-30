#pragma once

#include <windows.h>
#include <unknwn.h>

class ClassFactory : public IClassFactory {
public:
	// IUnknown
	IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
	IFACEMETHODIMP_(ULONG) AddRef();
	IFACEMETHODIMP_(ULONG) Release();

	// IClassFactory
	IFACEMETHODIMP CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);
	IFACEMETHODIMP LockServer(BOOL fLock);

	ClassFactory() noexcept;

	void *operator new(size_t size) noexcept {
		return HeapAlloc(GetProcessHeap(), 0, size);
	}
	void operator delete(void *memory) noexcept {
		if (memory != NULL)
			HeapFree(GetProcessHeap(), 0, memory);
	}

protected:
	~ClassFactory();

private:
	volatile long m_cRef;
};

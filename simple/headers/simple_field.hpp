#pragma once

#include "simple_dynamic_array.hpp"
#include "simple_logging.hpp"

namespace simple {

	template<typename FieldType>
	struct Reference_T {

		inline Reference_T() noexcept : _pField(nullptr) {}

		inline Reference_T(FieldType& field) noexcept : _pField(&field) {
			_pField->AddReference(*this);
		}

		inline Reference_T(Reference_T&& other) noexcept : _pField(other._pField) {
			if (_pField) {
				_pField->ChangeReference(other, *this);
			}
			other._pField = nullptr;
		}

		inline Reference_T(const Reference_T& other) noexcept : _pField(other._pField) {
			if (_pField) {
				_pField->AddReference(*this);
			}
		}

		inline bool IsNull() {
			return !_pField;
		}

		inline void SetField(FieldType& field) noexcept {
			_pField = &field;
			_pField->AddReference(*this);
		}

		inline FieldType& GetField() {
			assert(_pField && "attempting to call simple::Reference_T::GetField when the simple::Field is null!");
			return *_pField;
		}

		inline ~Reference_T() noexcept {
			if (_pField) {
				_pField->RemoveReference(*this);
			}
		}

	private:
		FieldType* _pField;
		friend typename FieldType;
	};

	template<typename T>
	struct Field {

		typedef Reference_T<Field> Reference;

		T value{};

		void AddReference(Reference& reference) {
			_references.PushBack(&reference);
		}

		bool RemoveReference(Reference& reference) {
			auto iter = _references.Find(&reference);
			if (iter != _references.end()) {
				_references.Erase(iter);
				return true;
			}
			return false;
		}

		bool ChangeReference(Reference& oldRef, Reference& newRef) {
			auto iter = _references.Find(&oldRef);
			if (iter != _references.end()) {
				*iter = &newRef;
				return true;
			}
			return false;
		}

		inline ~Field() {
			for (Reference* reference : _references) {
				reference->_pField = nullptr;
			}
		}

	private:
		DynamicArray<Reference*> _references;
	};
}

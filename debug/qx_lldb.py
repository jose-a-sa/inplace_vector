# LLDB data formatter for qx::inplace_vector<N, T>.
# Load with:  command script import /path/to/qx_lldb.py   (or from ~/.lldbinit)
import os
import re
import sys
import traceback

import lldb

_TYPE = r'^qx::inplace_vector<.*>$'


def _log(where):
    # lldb swallows exceptions of formatters: set QX_LLDB_DEBUG=1 to see them
    if os.environ.get('QX_LLDB_DEBUG'):
        sys.stderr.write('qx_lldb: %s failed:\n%s\n' % (where, traceback.format_exc()))


def _find_member(valobj, name):
    """Find a data member by name, looking through the (private) base classes and anonymous unions/structs."""
    child = valobj.GetChildMemberWithName(name)
    if child.IsValid():
        return child
    for i in range(valobj.GetNumChildren()):
        sub = valobj.GetChildAtIndex(i)
        sub_name = sub.GetName()
        if not sub_name or 'inplace_vector' in sub_name:  # an anonymous union/struct or a base class
            found = _find_member(sub, name)
            if found.IsValid():
                return found
    return lldb.SBValue()


def _elements(valobj):
    """The element array: `data_`, or `elements_.data_` in the layout with an intermediate member."""
    data = _find_member(valobj, 'data_')
    if not data.IsValid():
        elements = _find_member(valobj, 'arr_')
        data = elements.GetChildMemberWithName('data') if elements.IsValid() else lldb.SBValue()
    return data


def _template_element_type(valobj):
    t = valobj.GetType()
    if t.IsReferenceType():
        t = t.GetDereferencedType()
    if t.IsPointerType():
        t = t.GetPointeeType()
    return t.GetTemplateArgumentType(1)  # argument 0 is the capacity


def _capacity(valobj, data):
    """N of inplace_vector<N, T>: from the element array, or from the type's name when there is none (N == 0)."""
    if data.IsValid():
        element = data.GetType().GetArrayElementType()
        if element.IsValid() and element.GetByteSize():
            return data.GetType().GetByteSize() // element.GetByteSize()
    t = valobj.GetType()
    if t.IsReferenceType():
        t = t.GetDereferencedType()
    if t.IsPointerType():
        t = t.GetPointeeType()
    match = re.search(r'inplace_vector<(\d+)', t.GetCanonicalType().GetName() or t.GetName())
    return int(match.group(1)) if match else None


def _inspect(valobj):
    """(size, capacity, initialized, data) of a vector. sz_ is garbage until the constructor has run (a breakpoint inside
    it, an uninitialized local): it can never legitimately exceed the capacity."""
    size = _find_member(valobj, 'sz_')  # absent for inplace_vector<0, T>
    size = size.GetValueAsUnsigned() if size.IsValid() else 0
    data = _elements(valobj)
    capacity = _capacity(valobj, data)
    return size, capacity, capacity is None or size <= capacity, data


class InplaceVectorSynth:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.update()

    def update(self):
        self.size = 0
        self.address = lldb.LLDB_INVALID_ADDRESS
        self.data = None
        self.element_type = None
        self.stride = 0
        try:
            v = self.valobj  # lldb looks through references and pointers when asked for children
            size, _, initialized, data = _inspect(v)
            self.size = size if initialized else 0
            if data.IsValid():
                self.data = data
                self.address = data.GetLoadAddress()
                self.element_type = data.GetType().GetArrayElementType()
                if not self.element_type.IsValid() or self.element_type.GetByteSize() == 0:
                    self.element_type = _template_element_type(v)  # inplace_vector<N, T>: T
                self.stride = self.element_type.GetByteSize() if self.element_type.IsValid() else 0
        except Exception:
            _log('update')
        return False

    def has_children(self):
        return self.size > 0

    def num_children(self):
        return self.size

    def get_child_index(self, name):
        try:
            return int(name.lstrip('[').rstrip(']'))
        except ValueError:
            return -1

    def get_child_at_index(self, index):
        try:
            if self.data is None or not 0 <= index < self.size:
                return None
            name = '[%d]' % index
            if self.address != lldb.LLDB_INVALID_ADDRESS and self.stride:
                # an element built from its address and type, like lldb's own formatters do
                child = self.valobj.CreateValueFromAddress(name, self.address + index * self.stride, self.element_type)
                if child.IsValid():
                    return child
            return self.data.GetChildAtIndex(index)
        except Exception:
            _log('get_child_at_index(%d)' % index)
            return None


def summary(valobj, internal_dict):
    size, capacity, initialized, _ = _inspect(valobj.GetNonSyntheticValue())
    capacity = '?' if capacity is None else capacity
    if not initialized:
        return '(not initialized), capacity=%s' % (capacity, )
    return 'size=%d, capacity=%s' % (size, capacity)


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand('type synthetic add -x "%s" --python-class qx_lldb.InplaceVectorSynth' % _TYPE)
    debugger.HandleCommand('type summary add -x "%s" -e --python-function qx_lldb.summary' % _TYPE)
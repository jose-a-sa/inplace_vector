# GDB pretty-printer for qx::inplace_vector<N, T>.
# Load with:  source /path/to/qx_gdb.py   (or from ~/.gdbinit, or auto-load next to the binary)
import gdb
import gdb.printing


def _find_member(val, name):
    """Find a data member by name, looking through the (private) base-class chain."""
    for f in val.type.strip_typedefs().fields():
        if f.is_base_class:
            found = _find_member(val[f], name)
            if found is not None:
                return found
        elif f.name == name:
            return val[f]
    return None


class InplaceVectorPrinter:
    def __init__(self, val):
        if val.type.strip_typedefs().code in (gdb.TYPE_CODE_REF, gdb.TYPE_CODE_RVALUE_REF):
            val = val.referenced_value()  # gdb passes references as they are
        self.val = val
        t = val.type.strip_typedefs().unqualified()
        self.capacity = int(t.template_argument(0))
        sz = _find_member(val, 'sz_')  # absent for inplace_vector<0, T>
        self.size = int(sz) if sz is not None else 0

    def to_string(self):
        return 'size=%d, capacity=%d' % (self.size, self.capacity)

    def children(self):
        if self.size == 0:
            return
        data = _find_member(self.val, 'arr_')['data']
        for i in range(self.size):
            yield '[%d]' % i, data[i]

    def display_hint(self):
        return 'array'


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter('qx')
    pp.add_printer('inplace_vector', r'^qx::inplace_vector<.*>$', InplaceVectorPrinter)
    return pp


gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer(), replace=True)

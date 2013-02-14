#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .callbackhandler import CallbackHandler
from .members import Member


class SignalBinder(object):
    """ A class used by `Signal` to bind and invoke callbacks.

    There is a much higher performance version of this class available
    as a C++ extension. Prefer building Atom with this extension.

    """
    __slots__ = ('_handler',)

    def __init__(self):
        self._handler = CallbackHandler()

    def __call__(self, *args, **kwargs):
        self._handler.invoke_callbacks(*args, **kwargs)

    def emit(self, *args, **kwargs):
        self._handler.invoke_callbacks(*args, **kwargs)

    def connect(self, callback):
        self._handler.add_callback(callback)

    def disconnect(self, callback):
        self._handler.remove_callback(callback)


#: Use the faster C++ version if available
try:
    from .extensions.signalbinder import SignalBinder
except ImportError:
    pass


class Signal(Member):

    __slots__ = ()

    def __init__(self):
        self.has_default = True
        self.has_validate = True

    def default(self, owner, name):
        return SignalBinder()

    def validate(self, owner, name, old, new):
        raise TypeError('cannot assign to a signal')


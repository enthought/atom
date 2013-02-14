#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
from .catom import null
from .callbackhandler import CallbackHandler
from .members import Member


class EventBinder(object):
    """ A class used by `Event` to bind and invoke callbacks.

    There is a much higher performance version of this class available
    as a C++ extension. Prefer building Atom with this extension.

    """
    __slots__ = ('_handler',)

    def __init__(self):
        self._handler = CallbackHandler()

    def __call__(self, argument=null):
        if argument is not null:
            self._handler.invoke_callbacks(argument)
        else:
            self._handler.invoke_callbacks()

    def bind(self, callback):
        self._handler.add_callback(callback)

    def unbind(self, callback):
        self._handler.remove_callback(callback)


#: Use the faster C++ version if available
try:
    from .extensions.eventbinder import EventBinder
except ImportError:
    pass


class Event(Member):

    __slots__ = ()

    def __init__(self):
        self.has_default = True

    def default(self, owner, name):
        return EventBinder()

    def __set__(self, owner, value):
        self.__get__(owner, type(owner))(value)


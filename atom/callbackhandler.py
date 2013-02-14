#------------------------------------------------------------------------------
#  Copyright (c) 2013, Enthought, Inc.
#  All rights reserved.
#------------------------------------------------------------------------------
class CallbackHandler(object):
    """ A pure Python implementation of the C++ callback handler.

    """
    __slots__ = ('_callbacks', '_tasks')

    def __init__(self):
        self._callbacks = []
        self._tasks = None

    def add_callback(self, callback):
        tasks = self._tasks
        if tasks is not None:
            tasks.append(lambda: self.add_callback(callback))
            return
        callbacks = self._callbacks
        if callback not in callbacks:
            callbacks.append(callback)

    def remove_callback(self, callback):
        tasks = self._tasks
        if tasks is not None:
            tasks.append(lambda: self.remove_callback(callback))
            return
        callbacks = self._callbacks
        if callback in callbacks:
            callbacks.remove(callback)

    def invoke_callbacks(self, *args, **kwargs):
        owns_tasks = self._tasks is None
        if owns_tasks:
            self._tasks = []
        try:
            for callback in self._callbacks:
                if callback:
                    callback(*args, **kwargs)
                else:
                    self._tasks.append(lambda: self.remove_callback(callback))
        finally:
            if owns_tasks:
                tasks = self._tasks
                self._tasks = None
                for task in tasks:
                    task()


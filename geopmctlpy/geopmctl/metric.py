#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

'''
Defines Metric/Signal related classes and factory method(s).
'''

from enum import Enum
from abc import ABC
from abc import abstractmethod
import yaml
from geopmdpy import pio
from geopmdpy import topo


def create_signal(name, domain, instance, unit="", report_header=None,
                  trace_header=None, report_format=None, trace_format=None):
    '''
    Factory function for creating a Signal object.

    Args

    name (str): Name of the GEOPM signal.

    domain (geopmdpy.topo field): A domain enumeration from the geopmdpy.topo package

    instance (int): Instance number.

    unit (str): A string such as joules, seconds, etc. Default is empty string.

    report_header (str): A descriptor string that would go into a report file. Default is signal
    signature.

    trace_header (str): A header string that would go into a trace file. Default is signal
    signature.

    report_format (str or func): A format string with a single value or a function with
    signature func(x) that will that will be used to format the value of this metric in report
    output. If None, no formatting will be applied. No formatting by default.

    trace_format (str or func): A format string with a single value or a function with signature
    func(x) that will that will be used to format the value of this metric in trace output. If
    None, no formatting will be applied. No formatting by default.
    '''
    return_val = None
    behavior = Behavior(pio.signal_info(name)[2])
    if behavior is Behavior.MONOTONE:
        return_val = MonotoneSignal(name, domain, instance, unit=unit,
                                    report_header=report_header, trace_header=trace_header,
                                    report_format=report_format, trace_format=trace_format)
    else:
        return_val = Signal(name, domain, instance, behavior, unit=unit,
                            report_header=report_header, trace_header=trace_header,
                            report_format=report_format, trace_format=trace_format)
    return return_val


class Behavior(Enum):
    '''
    Enum representing Signal behaviors. Should match the integer representation of behaviors
    returned by pio.
    '''
    CONSTANT = 0
    MONOTONE = 1
    VARIABLE = 2
    LABEL = 3


class SignalList:
    '''
    Class storing a collection of signals. A signal list is a representative of the signals passed
    to the Agent.update() method.
    '''

    def __init__(self):
        self._signals = []

    def update(self, signal_values):
        '''
        Update all the signals with values from Agent.update().

        Args

        signal_values (list of float): Signals passed to Agent.update() by the Controller.
        '''
        for signal, value in zip(self._signals, signal_values):
            signal.update(value)

    def reset(self):
        '''
        Reset all the signals.
        '''
        for signal in self._signals:
            signal.reset()

    def append(self, signal):
        '''
        Appends a signal to the signal list

        Args

        signal (Signal): A Signal object.
        '''
        self._signals.append(signal)

    def get_tuples(self):
        '''
        Returns

        A list of signal.get_tuple() outputs which can be used in Agent.get_signals().
        '''
        return [sig.get_tuple() for sig in self._signals]

    def __getitem__(self, idx):
        '''
        Finds and returns a signal in this list via its index, tuple representation or signature
        string. Usage is through [] notation. Example: sig_list["name@board-0"].

        Args

        idx (int or tuple of (str, domain, int) or str): Index, tuple representation or signature
        string.
        '''
        return_val = None
        if isinstance(idx, int):
            return_val = self._signals[idx]
        elif isinstance(idx, tuple):
            for sig in self._signals:
                if sig.get_tuple() == idx:
                    return_val = sig
                    break
        elif isinstance(idx, str):
            for sig in self._signals:
                if sig.get_signature() == idx:
                    return_val = sig
                    break

        if return_val is None:
            raise ValueError("Not found: " + str(idx))

        return return_val


class MetricList:
    '''
    Class storing a collection of metrics. A metrics list is a representative of the metrics passed
    to an agent Algorithm.

    This class keeps track of the time metric in order to pass it to update method of
    TimeAvgAccumulators, if any.
    '''

    def __init__(self):
        self._metrics = []
        self._time_signal = None

    def append_signal(self, signal):
        '''
        Convenience method for adding a signal to the metric list as sample_only.

        Args

        signal (Signal): A Signal object.
        '''
        accumulator = None
        if signal.behavior() == Behavior.MONOTONE:
            accumulator = SumAccumulator()
        else:
            accumulator = TimeAvgAccumulator()
        metric = Metric([signal], Metric.OPERATIONS['sample_only'], accumulator,
                        signal.report_header(), signal.trace_header(), signal.unit(),
                        signal.report_format(), signal.trace_format())
        self.append_metric(metric)

    def append_metric(self, metric):
        '''
        Append a metric to the metric list.

        Args

        metric (Metric): A Metric object.
        '''
        self._metrics.append(metric)
        if len(metric.signals) == 1 and metric.signals[0].get_signature() == "TIME@board-0":
            self._time_signal = metric.signals[0]

    def update(self):
        '''
        Update all metrics in this list. Should be called only after Signal(List).update() calls
        for all the signals that affect the metrics.
        '''
        for metric in self._metrics:
            metric.update(self._time_signal)

    def get_report_str(self):
        '''
        Returns

        A string that would be printed in a report.
        '''
        out_dict = {metric.get_report_header(): metric.get_accumulator_value_str()
                    for metric in self._metrics}
        return yaml.dump(out_dict)

    def get_trace_row(self):
        '''
        Returns the metric values for the sample interval formatted according to metric trace
        format.

        Returns

        List of str: One element per metric.get_trace_value_str()
        '''
        return [metric.get_trace_value_str() for metric in self._metrics]


class Signal:
    '''
    Signal class stores value for a single GEOPM signal which is identified by a name, domain
    and instance. Values should be updated at every sample interval via the update() method. This
    implementation will pass the signal values from update() to value() as is.
    '''

    def __init__(self, name, domain, instance, behavior, unit="", report_header=None,
                 trace_header=None, report_format=None, trace_format=None):
        '''
        Args

        name (str): Name of the GEOPM signal.

        domain (geopmdpy.topo field): A domain enumeration from the geopmdpy.topo package

        instance (int): Instance number.

        behavior (Behavior enum): Behavior of the signal.

        unit (str): A string such as joules, seconds, etc. Default is empty string.

        report_header (str): A descriptor string that would go into a report file. Default is signal
        signature.

        trace_header (str): A header string that would go into a trace file. Default is signal
        signature.

        report_format (str or func): A format string with a single value or a function with
        signature func(x) that will that will be used to format the value of this metric in report
        output. If None, no formatting will be applied. No formatting by default.

        trace_format (str or func): A format string with a single value or a function with signature
        func(x) that will that will be used to format the value of this metric in trace output. If
        None, no formatting will be applied. No formatting by default.
        '''
        self._name = name
        self._domain = domain
        self._instance = instance
        self._behavior = behavior
        self._unit = unit
        self._report_header = self.get_signature() if report_header is None else report_header
        self._trace_header = self.get_signature() if trace_header is None else trace_header
        self._report_format = report_format
        self._trace_format = trace_format
        self._value = None

    def get_name(self):
        '''
        Getter method for Signal name.
        '''
        return self._name

    def get_domain(self):
        '''
        Getter method for Signal domain.
        '''
        return self._domain

    def get_instance(self):
        '''
        Getter method for Signal instance.
        '''
        return self._instance

    def get_behavior(self):
        '''
        Getter method for Signal behavior.
        '''
        return self._behavior

    def get_unit(self):
        '''
        Getter method for Signal unit.
        '''
        return self._unit

    def get_report_header(self):
        '''
        Getter method for Signal report header.
        '''
        return self._report_header

    def get_trace_header(self):
        '''
        Getter method for Signal trace header.
        '''
        return self._trace_header

    def get_report_format(self):
        '''
        Getter method for Signal report format.
        '''
        return self._report_format

    def get_trace_format(self):
        '''
        Getter method for Signal trace format.
        '''
        return self._trace_format

    def value(self):
        '''
        Returns

        Computed value for this signal according to the last update value. None, if there is not
        enough information (for example, update() is not called yet).
        '''
        return self._value

    def reset(self):
        '''
        Return to a state where update() has not been called yet.
        '''
        self._value = None

    def update(self, value):
        '''
        Update the value of the signal from a sample.

        Args

        value (float): Value returned by pio.
        '''
        self._value = value

    def get_tuple(self):
        '''
        Returns

        A tuple of (name, domain, instance), which can be used in Agent.get_signals()
        '''
        return self._name, self._domain, self._instance

    def get_signature(self):
        '''
        Returns

        A string of name@domain-instance.
        '''
        return f'{self._name}@{topo.domain_name(self._domain)}-{self._instance}'


class MonotoneSignal(Signal):
    '''
    Implementation of Signal where the computed value is delta between the last sample and current
    sample for monotone signals.
    '''

    def __init__(self, name, domain, instance, unit="", report_header=None,
                 trace_header=None, report_format=None, trace_format=None):
        self._last_value = None
        super().__init__(name, domain, instance, Behavior.MONOTONE, unit=unit,
                         report_header=report_header, trace_header=trace_header,
                         report_format=report_format, trace_format=trace_format)

    def reset(self):
        self._last_value = None
        super().reset()

    def update(self, value):
        if self._last_value is None:
            self._last_value = value
        else:
            self._value = value - self._last_value
            self._last_value = value


class Algorithm(ABC):
    '''
    A class that represents an algorithm that runs in an Agent. the inputs to an Algorithm are
    passed via a MetricList object. Algorithm processes the metrics and returns a list of floats
    for control values.
    '''

    def __init__(self, metric_list):
        '''
        Args

        metric_list (MetricList): A MetricList object that represents the input values to the
        algorithm.
        '''
        self._metric_list = metric_list
        self._controls = None

    @abstractmethod
    def update(self):
        '''
        Update the algorithm state with the latest metric values. Should be called after call to
        MetricList.update()
        '''

    def get_metric_list(self):
        '''
        Returns

        The MetricList object stored in this instance.
        '''
        return self._metric_list

    def get_controls(self):
        '''
        Returns

        List of float: The controls that should be returned by Agent.update().
        '''
        return self._controls


class Metric:
    '''
    Instances of this class keeps track of signal values or values that are derived from multiple
    signals. A metric keeps information on how to print its values in traces and reports.

    This class carries the reponsibility to accumulate the values over time to report at the end.
    '''

    OPERATIONS = {
        'sample_only': lambda x: x,
        'divide': lambda x, y: x / y,
        'multiply': lambda x, y: x * y,
    }

    def __init__(self, signals, operation, accumulator, report_header, trace_header, unit="",
                 report_format=None, trace_format=None):
        '''

        Args

        signals (list of Signal): One or more Signal instances that go into creating this metric
        via operation provided via the operation argument.

        operation (str): One of keys from Metric.OPERATIONS.

        accumulator (Accumulator obj): An accumulator for the final reported value.

        report_header (str): A descriptor string that would go into a report file.

        trace_header (str): A header string that would go into a trace file.

        unit (str): A string such as joules, seconds, etc. Default is empty string.

        report_format (str or func): A format string with a single value or a function with
        signature func(x) that will that will be used to format the value of this metric in report
        output. If None, no formatting will be applied.

        trace_format (str or func): A format string with a single value or a function with signature
        func(x) that will that will be used to format the value of this metric in trace output. If
        None, no formatting will be applied.
        '''
        self._report_header = report_header
        self._signals = signals
        self._operation = operation
        self._accumulator = accumulator
        self._trace_header = trace_header
        self._unit = unit
        self._report_format = report_format
        self._trace_format = trace_format
        self._value = None
        self.reset()

    def reset(self):
        '''
        Reset the stored value of the metric.
        '''
        self._accumulator.reset()
        self._value = None

    def update(self, time_signal=None):
        '''
        Updates the metric value from the signals. Signal.update() needs to be called for all
        signals in this metric prior to this call.

        Args

        time_signal (Signal): The signal object that represents time. Only required if there is
        a metric accumulator of type TimeAvgAccumulator.
        '''
        self._value = self._operation([sig.value() for sig in self._signals])
        if self._accumulator is not None:
            delta_time = None if time_signal is None else time_signal.value()
            self._accumulator.update(self._value, delta_time)

    def value(self):
        '''
        Returns

        The current value of this metric after the past update() call.
        '''
        return self._value

    def accumulator_value(self):
        '''
        Returns

        The current value of this metric accumulator after the past update() call.
        '''
        return_val = None
        if self._accumulator is not None:
            return_val = self._accumulator.value()

        return return_val

    def get_trace_value_str(self):
        '''
        Returns

        Value formatted as indicated by trace_format.
        '''
        return_val = None
        if self._trace_format is None:
            return_val = self.value()
        elif callable(self._trace_format):
            return_val = self._trace_format(self.value())
        else:
            return_val = self._trace_format.format(self.value())
        return return_val

    def get_accumulator_value_str(self):
        '''
        Returns

        Accumulator value formatted as indicated by report_format and unit appended.
        '''
        return_val = None
        acc_value = self.accumulator_value()
        if acc_value is not None:
            if self._report_format is None:
                return_val = acc_value
            elif callable(self._report_format):
                return_val = self._report_format(acc_value)
            else:
                return_val = self._report_format.format(acc_value)
            return_val += self._unit
        return return_val

    def get_report_header(self):
        '''
        Returns

        The descriptor string that would go into a report file.
        '''
        return self._report_header

    def get_trace_header(self):
        '''
        Returns

        The header string that would go into a trace file.
        '''
        return self._trace_header


class Accumulator(ABC):
    '''
    Accumulators are used by Metric instances in order to keep track of values for the final report.
    An implementing class defines a method to accumulate values such as sum, average, etc.
    '''

    @abstractmethod
    def reset(self):
        '''
        Reset the accumulator to a state where update() has never been called.
        '''

    @abstractmethod
    def update(self, value, delta_time):
        '''
        Update the accumulator with a new sample.

        Args

        value (float): Value of the metric for the sampling interval.

        delta_time (float): Duration of the sampling interval.
        '''

    @abstractmethod
    def value(self):
        '''
        Returns

        The accumulated value via the update() calls after the last call to reset().
        '''


class AvgAccumulator(Accumulator):
    '''
    Average accumulator divides the total value of all update calls by the number of calls to
    update().
    '''

    def __init__(self):
        self._acc_value = None
        self._num_calls = None
        self.reset()

    def reset(self):
        self._acc_value = 0
        self._num_calls = 0

    def update(self, value, delta_time):
        self._acc_value += value
        self._num_calls += 1

    def value(self):
        return self._acc_value / self._num_calls


class TimeAvgAccumulator(Accumulator):
    '''
    Time averages accumulator takes the time-weighted average of all calls to update().
    update().
    '''

    def __init__(self):
        self._acc_value = None
        self._total_time = None
        self.reset()

    def reset(self):
        self._acc_value = 0
        self._total_time = 0

    def update(self, value, delta_time):
        self._acc_value += (value * delta_time)
        self._total_time += delta_time

    def value(self):
        return self._acc_value / self._total_time


class SumAccumulator(Accumulator):
    '''
    Sum accumulator sums the values of all update calls.
    '''

    def __init__(self):
        self._acc_value = None
        self.reset()

    def reset(self):
        self._acc_value = 0

    def update(self, value, delta_time):
        self._acc_value += value

    def value(self):
        return self._acc_value

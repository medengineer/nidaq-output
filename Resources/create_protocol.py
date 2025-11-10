from opto_stim_schema import Protocol, Sequence, Stimulus, Condition, PulseShape

from pydantic_yaml import to_yaml_str

my_stimulus1 = Stimulus(
    source="External laser",
    site=-1,
    wavelength=473,
    power=100,
    duration=1.0,
    pulse_shape="Square",
    pulse_width=0.01,
    pulse_frequency=20,
)

my_stimulus2 = Stimulus(
    source="Probe A",
    site=10,
    wavelength=638,
    power=50,
    pulse_shape="Square",
    pulse_width=0.01,
    duration=1.0,
    pulse_frequency=20,
)

my_stimulus3 = Stimulus(
    source="Probe B",
    site=5,
    wavelength=638,
    power=50,
    pulse_width=0.10,
    pulse_shape="Custom",
    custom_waveform=[1, 2, 3, 4],
    duration=1.0,
    pulse_duration=1,
    pulse_frequency=0,
)

my_condition1 = Condition(num_repeats=10, stimuli=[my_stimulus1])

my_condition2 = Condition(num_repeats=5, stimuli=[my_stimulus2, my_stimulus3])

my_sequence = Sequence(min_iti=1, max_iti=1, conditions=[my_condition1, my_condition2])

my_protocol = Protocol(name="My Protocol", sequences=[my_sequence])

with open("protocol.yml", "w") as yaml_file:
    yaml_file.write(to_yaml_str(my_protocol, indent=2))

import math
from typing import Iterable, Tuple
from itertools import takewhile, chain
from delta_debugger import run_target

def split_delta(input: bytes, chunks: int) -> Iterable[Tuple[bytes, int]]:
    chunk_size = math.ceil(len(input) / chunks)
    deltas = [(input[i:i+chunk_size], min(2, chunk_size)) for i in range(0, len(input), chunk_size)]
    yield from deltas
    yield from ((b''.join(i[0] for i in deltas[:i] + deltas[i+1:]), chunks-1) for i in range(chunks))

def iterate(f, x):
    while True:
        yield x
        x = f(x)

def delta_debug(target: str, input: bytes, split: int = 2) -> bytes:
    """
    Delta-Debugging algorithm

    TODO: Implement your algorithm for delta-debugging.

    Hint: It may be helpful to use an auxiliary function that
    takes as input a target, input string, n and
    returns the next input and n to try.

    :param target: target program
    :param input: crashing input to be minimized
    :return: 1-minimal crashing input.
    """
    if run_target(target, input) == 0:
        return None
    elif split <= 1:
        return input
    else:
        input_length = len(input)
        splits = takewhile(lambda split: split <= input_length, iterate(lambda n: n * 2, split))
        candidates = chain([(b'', 0)], chain.from_iterable(split_delta(input, split) for split in splits))
        shrinked_candidates = (delta_debug(target, candidate_input, split) for candidate_input, split in candidates)
        return next(filter(lambda i: i is not None, shrinked_candidates), input)

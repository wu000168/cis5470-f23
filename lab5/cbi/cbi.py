#! /usr/bin/env python3

from collections import defaultdict
import itertools
import functools
from pathlib import Path
from typing import Dict, Iterable, List, Set
from cbi.data_format import (
    CBILog,
    ObservationStatus,
    Predicate,
    PredicateInfo,
    PredicateType,
    Report,
)
from cbi.utils import get_logs


def collect_observations(log: CBILog) -> Dict[Predicate, ObservationStatus]:
    """
    Traverse the CBILog and collect observation status for each predicate.

    NOTE: If you find a `Predicate(line=3, column=5, pred_type="BranchTrue")`
    in the log, then you have observed it as True,
    further it also means you've observed is complement:
    `Predicate(line=3, column=5, pred_type="BranchFalse")` as False.

    :param log: the log
    :return: a dictionary of predicates and their observation status.
    """
    observations: Dict[Predicate, ObservationStatus] = defaultdict(
        lambda: ObservationStatus.NEVER
    )

    """
    TODO: Add your code here

    Hint: The PredicateType.alternatives will come in handy.
    """
    for entry in log:
        for pred_type, obs in PredicateType.alternatives(entry.value):
            pred = Predicate(entry.line, entry.column, pred_type)
            observations[pred] = ObservationStatus.merge(observations[pred], obs) 
        
    return observations


def collect_all_predicates(logs: Iterable[CBILog]) -> Set[Predicate]:
    """
    Collect all predicates from the logs.

    :param logs: Collection of CBILogs
    :return: Set of all predicates found across all logs.
    """
    predicates = set(
        Predicate(entry.line, entry.column, pred_type) 
            for log in logs for entry in log
                for pred_type, _ in PredicateType.alternatives(entry.value)
    )

    return predicates


def cbi(success_logs: List[CBILog], failure_logs: List[CBILog]) -> Report:
    """
    Compute the CBI report.

    :param success_logs: logs of successful runs
    :param failure_logs: logs of failing runs
    :return: the report
    """
    all_predicates = collect_all_predicates(itertools.chain(success_logs, failure_logs))

    predicate_infos: Dict[Predicate, PredicateInfo] = {
        pred: PredicateInfo(pred) for pred in all_predicates
    }

    # TODO: Add your code here to compute the information for each predicate.
    for log in success_logs:
        for pred, status in collect_observations(log).items():
            if status == ObservationStatus.ONLY_TRUE or status == ObservationStatus.BOTH:
                predicate_infos[pred].s += 1
            if status != ObservationStatus.NEVER:
                predicate_infos[pred].s_obs += 1

    for log in failure_logs:
        for pred, status in collect_observations(log).items():
            if status == ObservationStatus.ONLY_TRUE or status == ObservationStatus.BOTH:
                predicate_infos[pred].f += 1
            if status != ObservationStatus.NEVER:
                predicate_infos[pred].f_obs += 1


    # Finally, create a report and return it.
    report = Report(predicate_info_list=list(predicate_infos.values()))
    return report

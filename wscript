## -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    module = bld.create_ns3_module('point-to-point-layout', ['internet', 'point-to-point', 'mobility', 'netanim'])
    module.includes = '.'
    module.source = [
        'model/point-to-point-dumbbell.cc',
        'model/point-to-point-grid.cc',
        'model/point-to-point-star.cc',

        'ccdn/fat-tree-helper.cc',
        'ccdn/content-fib-entry.cc',
        'ccdn/content-fib.cc',
        'ccdn/content-cache.cc',
        'ccdn/mix-routing.cc',
        'ccdn/ipv4-mix-routing-helper.cc',
        'ccdn/global-content-manager.cc',
        'ccdn/data-transfer.cc',
        'ccdn/task-recorder.cc',
        ]

    headers = bld.new_task_gen(features=['ns3header'])
    headers.module = 'point-to-point-layout'
    headers.source = [
        'model/point-to-point-dumbbell.h',
        'model/point-to-point-grid.h',
        'model/point-to-point-star.h',

        'ccdn/fat-tree-helper.h',
        'ccdn/content-fib-entry.h',
        'ccdn/content-fib.h',
        'ccdn/content-cache.h',
        'ccdn/mix-routing.h',
        'ccdn/ipv4-mix-routing-helper.h',
        'ccdn/global-content-manager.h',
        'ccdn/data-transfer.h',
        'ccdn/task-recorder.h',
        'ccdn/parameter.h',
        ]

    bld.ns3_python_bindings()




#!/usr/bin/python3

from collections import OrderedDict
import yaml
import re
import sys

# https://dotnet.github.io/docfx/tutorial/links_and_cross_references.html


def DumpOrderedDict(dumper, data):
    return dumper.represent_mapping(
            yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
            data.items())


class DocFxDumper(yaml.Dumper):
    pass


DocFxDumper.add_representer(OrderedDict, DumpOrderedDict)


def LoadOrderedDict(loader, data):
    loader.flatten_mapping(data)
    return OrderedDict(loader.construct_pairs(data))


class DocFxLoader(yaml.SafeLoader):
    pass


DocFxLoader.add_constructor(
        yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
        LoadOrderedDict)


def generate_xrefmap(paths):
    xreflist = []
    xrefmap = {}
    for path in paths:
        if re.match(r"(^|.*/)toc\.yml$", path):
            continue
        print(path, file=sys.stderr)
        with open(path, 'r') as fh:
            content = yaml.load(fh, DocFxLoader)
        for item in content['items']:
            ref = OrderedDict([
                ('uid', item['uid']),
                ('name', item['name']),
                ('href', '/' + re.sub('.yml', '.html', path)),
                ('fullName', item['fullName'])
            ])
            xreflist += [ref]
            xrefmap[item['uid']] = ref

    with open('xrefmap.yml', 'w') as fh:
        fh.write(yaml.dump({'references': xreflist}, None, DocFxDumper))

    for path in paths:
        if re.match(r"(^|.*/)toc\.yml$", path):
            continue
        with open(path, 'r') as fh:
            content = yaml.load(fh, DocFxLoader)
        content['references'] = []
        referenced = set()
        for item in content['items']:
            if 'parent' in item:
                if item['parent'] in xrefmap:
                    referenced.add(item['parent'])
                else:
                    del item['parent']
            if 'children' in item:
                item['children'] = [
                        c for c in item['children'] if c in xrefmap]
                for child in item['children']:
                    referenced.add(child)
            if 'inheritedMembers' in item:
                item['inheritedMembers'] = [
                        c for c in item['inheritedMembers'] if c in xrefmap]
                for child in item['inheritedMembers']:
                    referenced.add(child)
        for reference in referenced:
            content['references'] += [xrefmap[reference]]
        with open(path, 'w') as fh:
            fh.write("### YamlMime:ManagedReference\n")
            fh.write(yaml.dump(content, None, DocFxDumper))


def main():
    generate_xrefmap(sys.argv[1:])


if __name__ == "__main__":
    main()

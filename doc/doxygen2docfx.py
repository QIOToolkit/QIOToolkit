#!/usr/bin/python3

from collections import OrderedDict
import os
import re
import sys
import xml.sax
import yaml


def RepresentOrderedDict(dumper, data):
    return dumper.represent_mapping(
            yaml.resolver.BaseResolver.DEFAULT_MAPPING_TAG,
            data.items())


class DocFxDumper(yaml.Dumper):
    pass


DocFxDumper.add_representer(OrderedDict, RepresentOrderedDict)


def translate(doxy):
    translations = {
        "C++": "cpp",
        "variable": "field",
    }

    if doxy in translations:
        return translations[doxy]
    else:
        return doxy


def camel2dashed(name):
    def replacement(match):
        return '-'+match.group(0).lower()
    name = re.sub('[A-Z]+', replacement, name)
    name = re.sub('_-', '_', name)
    return name.strip('-')


def id_sorting(item_id):
    return re.sub(r'^(class|struct|namespace)', '', item_id)


def has_tag(entity, tag):
    if entity is None or not hasattr(entity, 'tag'):
        return False
    return entity.tag == tag


class TagHandler(object):
    @staticmethod
    def create(tag, attrs, parent):
        attrs = dict(attrs.items())
        assert("text" not in attrs)
        assert("tag" not in attrs)
        assert("parent" not in attrs)
        attrs["tag"] = tag
        attrs["parent"] = parent
        attrs["text"] = None
        if tag in TagHandler.handlers:
            return TagHandler.handlers[tag](**attrs)
        else:
            return TagHandler(**attrs)

    def __init__(self, **attrs):
        for key in attrs:
            setattr(self, key, attrs[key])

    def enter(self, items):
        pass

    def leave(self, items):
        pass

    def characters(self, content, items):
        self.add_content(content)

    def add_content(self, content):
        if self.text is not None:
            self.text += content
        elif self.parent is not None:
            self.parent.add_content(content)
        else:
            pass

    def get_content(self):
        if self.text is not None:
            return self.text
        if self.parent is None:
            return None
        return self.parent.get_content()

    def ancestor(self, count):
        assert(count >= 0)
        if count == 0:
            return self
        if self.parent is None:
            return None
        return self.parent.ancestor(count-1)



TagHandler.handlers = {}

def tag(name):
    def register_tag_handler(cls):
        TagHandler.handlers[name] = cls
    return register_tag_handler


@tag("name")
class Name(TagHandler):
    def characters(self, content, items):
        if has_tag(self.ancestor(2), 'listofallmembers'):
            parent_id = self.ancestor(3).id
            member_id = self.parent.refid
            if parent_id in items and (
                    'children' not in items[parent_id]
                    or member_id not in items[parent_id]['children']):
                inherited = []
                if 'inheritedMembers' in items[parent_id]:
                    inherited = items[parent_id]['inheritedMembers']
                inherited += [member_id]
                inherited = sorted(set(inherited), key=id_sorting)
                items[parent_id]['inheritedMembers'] = inherited
        elif hasattr(self.ancestor(3), 'id'):
            parent_id = self.ancestor(3).id
            item_id = self.parent.id
            if 'children' not in items[parent_id]:
                items[parent_id]['children'] = []
            items[parent_id]['children'] += [item_id]
            if item_id in items:
                items[item_id]['name'] = content
                items[item_id]['fullName'] = (
                        items[parent_id]['fullName'] + '::' + content)
                if items[item_id]['type'] == 'function':
                    parent_name = items[parent_id]['name']
                    item_type = 'method'
                    if content == re.sub(".*::", "", parent_name):
                        item_type = 'constructor'
                    items[item_id]['type'] = translate(item_type)

@tag("definition")
class Definition(TagHandler):
    def characters(self, content, items):
        item_id = self.parent.id
        if item_id in items:
            if 'syntax' not in items[item_id]:
                items[item_id]['syntax'] = {'content': ''}
            items[item_id]['syntax']['content'] += content.strip()

@tag("argsstring")
class ArgsString(TagHandler):
    def characters(self, content, items):
        item_id = self.parent.id
        if item_id in items:
            if 'syntax' not in items[item_id]:
                items[item_id]['syntax'] = {'content': ''}
            items[item_id]['syntax']['content'] += content.strip()

@tag("listitem")
class ListItem(TagHandler):
    def enter(self, items):
        self.add_content("  * ")

@tag("para")
class Para(TagHandler):
    def characters(self, content, items):
        content = re.sub(r"^ *(NOTE|WARNING|TIP|IMPORTANT|CAUTION):", "\n> [!\\1]\n> ", content, count=1)
        self.add_content(content)

@tag("compounddef")
class CompoundDef(TagHandler):
    def enter(self, items):
        if self.kind != 'dir':
            item_id = self.id
            items[item_id] = OrderedDict([
                ('uid', item_id),
                ('id', item_id),
                ('type', translate(self.kind)),
                ('summary', ''),
            ])
            if hasattr(self, 'language'):
                language = translate(self.language)
                items[item_id]['langs'] = [language]

@tag("compoundname")
class CompountName(TagHandler):
    def characters(self, content, items):
        item_id = self.parent.id
        if item_id in items:
            items[item_id]['name'] = content
            items[item_id]['fullName'] = content

@tag("basecompoundref")
class BaseCompoundRef(TagHandler):
    def enter(self, items):
        item_id = self.parent.id
        if hasattr(self, 'refid'):
            if 'inheritance' not in items[item_id]:
                items[item_id]['inheritance'] = []
            items[item_id]['inheritance'] += [self.refid]

@tag("derivedcompoundref")
class DerivedCompoundRef(TagHandler):
    def enter(self, items):
        item_id = self.parent.id
        if 'derivedClasses' not in items[item_id]:
            derived = []
        else:
            derived = items[item_id]['derivedClasses']
        derived += [self.refid]
        items[item_id]['derivedClasses'] = sorted(set(derived), key=id_sorting)

@tag("memberdef")
class MemberDef(TagHandler):
    def enter(self, items):
        if self.kind not in ('friend', 'type', 'typedef', 'variable'):
            parent_id = self.ancestor(2).id
            member_id = self.id
            items[member_id] = OrderedDict([
                ('uid', member_id),
                ('id', member_id),
                ('parent', parent_id),
                ('type', translate(self.kind)),
                ('summary', ''),
            ])

@tag("computeroutput")
class ComputerOutput(TagHandler):
    def enter(self, items):
        self.is_closed = False
        self.add_content("`")

    def leave(self, items):
        if not self.is_closed:
            self.add_content("`")

@tag("programlisting")
class ProgramListing(TagHandler):
    def enter(self, items):
        self.text = ''

    def get_filename(self):
        if hasattr(self, 'filename'):
            return self.filename
        return ""

    def leave(self, items):
        INFERRED_CPP = " {c++}\n"
        if self.text.startswith(INFERRED_CPP):
            self.text = self.text[len(INFERRED_CPP):]
            self.filename = "cpp"
        if self.get_filename() == "c++":
            self.filename = "cpp"
        self.parent.add_content("\n```" + self.get_filename() + "\n" + self.text + "```\n")
        self.text = None

@tag("sp")
class Sp(TagHandler):
    def enter(self, items):
        self.add_content(" ")

@tag("briefdescription")
class BriefDescription(TagHandler):
    def enter(self, items):
        self.text = ''

    def leave(self, items):
        item_id = self.parent.id
        if item_id in items:
            items[item_id]['brief'] = self.text.strip()
            items[item_id]['summary'] = self.text.strip()
        self.text = None

@tag("detaileddescription")
class DetailedDescription(TagHandler):
    def enter(self, items):
        self.text = ''

    def leave(self, items):
        item_id = self.parent.id
        if item_id in items:
            items[item_id]['description'] = self.text.strip()
            items[item_id]['summary'] = (
                    items[item_id]['summary'] + "\n\n"
                    + self.text.strip()).strip()
        self.text = None

@tag("ref")
class Ref(TagHandler):
    def enter(self, items):
        if has_tag(self.parent, 'computeroutput'):
            node = self.parent
            while node is not None and node.text is None:
                node = node.parent
            if node is not None:
                node.text = node.text[:-1] + '[' + node.text[-1:]
        else:
            self.add_content('[')

    def leave(self, items):
        if has_tag(self.parent, 'computeroutput'):
            self.add_content('`')
            self.parent.is_closed = True
        self.add_content('](xref:' + self.refid + ')')


@tag("location")
class Location(TagHandler):
    def enter(self, items):
        item_id = self.parent.id
        if item_id in items and hasattr(self, 'bodyfile'):
            source = OrderedDict()
            # repo
            # branch
            # revision
            source['path'] = self.bodyfile
            source['startLine'] = int(self.bodystart)
            if hasattr(self, 'bodyend') and self.bodyend != '-1':
                source['endLine'] = int(self.bodyend)
            items[item_id]['source'] = source

@tag("formula")
class Formula(TagHandler):
    def enter(self, items):
        self.text = ""

    def leave(self, items):
        content = self.get_content()
        replacements = {
            r'^\s*\\\[(.*)\\\]\s*$': '\\1',
            r'^\s*\$(.*)\$\s*': '\\1'
        }
        for pattern, replace in replacements.items():
            if re.match(pattern, content):
                content = re.sub(pattern, replace, content)
                break
        self.parent.add_content("\n```math\n" + content + "\n```\n")
        print(content, file=sys.stderr)
        self.text = None


@tag("reimplementedby")
class ReimplementedBy(TagHandler):
    def enter(self, items):
        item_id = self.parent.id
        if item_id in items:
            if 'overrides' not in items[item_id]:
                items[item_id]['overrides'] = []
            items[item_id]['overrides'] += [self.refid]



class DocFxHandler(xml.sax.ContentHandler):
    def __init__(self):
        self.stack = []
        self.items = {}
        self.text = None
        self.uid_map = {}

    def translate_path(self, path):
        path = re.sub(r"doxygen/xml/(struct|class|namespace)(.*)\.xml",
                      "\\2.yml", path)
        path = re.sub(r"1_1", "", path)
        if re.match(r"(qiotoolkit)_", path):
            path = re.sub("qiotoolkit_", "", path)
        path = camel2dashed(path)
        # path = re.sub("_", "/", path, 1)
        path = re.sub('_', '/', path)
        return 'api/' + path

    def translate(self, doxy):
        if doxy in self.translations:
            return self.translations[doxy]
        else:
            return doxy

    def getYaml(self):
        docfx = {'items': list(self.items.values())}
        docfx_yaml = yaml.dump(docfx, None, DocFxDumper)
        return "### YamlMime:UniversalReference\n" + docfx_yaml

    def startElement(self, tag, attributes):
        parent = self.stack[-1] if len(self.stack) else None
        self.stack += [TagHandler.create(tag, attributes, parent)]
        self.stack[-1].enter(items=self.items)

    def endElement(self, tag):
        self.stack[-1].leave(items=self.items)
        self.stack = self.stack[:-1]

    def characters(self, content):
        self.stack[-1].characters(content, self.items)


def convert(path):
    converter = xml.sax.make_parser()
    doc_fx_handler = DocFxHandler()
    converter.setContentHandler(doc_fx_handler)
    converter.parse(path)
    if re.search('/namespacematcher', path):
        namespace = doc_fx_handler.items;
        namespace_name = None
        for key, items in namespace.items():
            if items['type'] == 'namespace':
                namespace_name = key[len("namespace"):]
            elif items['type'] == 'method':
                function_name = items["name"]
                if function_name == '&': continue
                filename = os.path.join('api', namespace_name, camel2dashed(function_name) + '.yml')
                print('converting', path, '->', filename, file=sys.stderr)
                doc_fx_handler.items = {items['uid']: items}
                os.makedirs(os.path.dirname(filename), exist_ok=True)
                with open(filename, 'w') as fh:
                    fh.write(doc_fx_handler.getYaml())
    elif re.search('/namespace', path):
        pass
    else:
        filename = doc_fx_handler.translate_path(path)
        if (filename == 'api/std/optional.yml' or
                filename =='api/ValueSetter/3_01std_optional_3_01T_01_4_01_4.yml'):
            pass
        else:
            print("converting", path, "->", filename, file=sys.stderr)
            os.makedirs(os.path.dirname(filename), exist_ok=True)
            with open(filename, 'w') as fh:
                fh.write(doc_fx_handler.getYaml())


def main(argv):
    for path in argv[1:]:
        convert(path)


if __name__ == "__main__":
    main(sys.argv)

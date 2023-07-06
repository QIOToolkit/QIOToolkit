
#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "error_handling.h"
#include "exception.h"
#include "problem.pb.h"
#include "stream_handler_proto.h"

namespace utils
{
std::unordered_map<QuantumUtil::Problem_ProblemType, std::string> model_type =
    {{QuantumUtil::Problem_ProblemType_PUBO, "pubo"},
     {QuantumUtil::Problem_ProblemType_ISING, "ising"},
     {QuantumUtil::Problem_ProblemType_MAXSAT, "maxsat"},
     {QuantumUtil::Problem_ProblemType_SOFTSPIN, "softspin"}};

class ProtoReader
{
 public:
  ProtoReader() {}

  template <typename StreamHandler>
  bool Parse(const std::string& folder_path,
             utils::PROTOHandlerProxy<StreamHandler>& handler_proxy)
  {
    unsigned file_count = 0;
    unsigned term_count = 0;
    QuantumUtil::Problem problem;
    std::fstream file_handler;

    // extract foldername from the path
    std::string folder_name;
    unsigned found = folder_path.find_last_of("/\\");
    if (found)
    {
      folder_name = folder_path.substr(found + 1);
    }
    else
    {
      // The folder is in the working directory
      folder_name = folder_path;
    }

    handler_proxy.StartObject();  // Start the problem dir read
    // Proto files are constructed internally and we control the naming
    // The naming schema is problemname_pb for folder and
    // problemname_pb_<filecount>.pb for individual messages problenname is
    // specified while constructing a problem on the client
    // Eg:: OptimizationProblem_pb\OptimzationProblem_pb_0.pb
    std::string file_name = folder_path + "/" + folder_name + "_" +
                            std::to_string(file_count) + ".pb";
    file_handler.open(file_name, std::ios::in | std::ios::binary);

    do
    {
      if (file_handler.fail())
      {
        THROW(utils::FileReadException, "Could not open file: ", file_name);
      }
      problem.ParseFromIstream(&file_handler);
      if (problem.has_cost_function())
      {
        const QuantumUtil::Problem_CostFunction& cost_function =
            problem.cost_function();
        const google::protobuf::RepeatedPtrField<QuantumUtil::Problem_Term>&
            terms = cost_function.terms();
        if (file_count == 0)  // Read the first file with the version, type and
                              // start of terms array
        {
          handler_proxy.StartObject();  // Start the cost function
          handler_proxy.Key("cost_function");

          QuantumUtil::Problem_ProblemType type = cost_function.type();
          if (model_type.find(type) == model_type.end())
          {
            THROW(
                utils::ValueException,
                "Expected type to be ising, pubo, maxsat or softspin. Invalid "
                "problem type specified. ");
          }

          std::string version = cost_function.version();
          if (type == QuantumUtil::Problem_ProblemType_SOFTSPIN)
          {
            if (version != "0.1" && !version.empty())
            {
              THROW(utils::ValueException,
                    "Expected version to be 0.1 for soft spin. Provided: ",
                    version);
            }
          }
          else if (version != "1.0" && version != "1.1" && !version.empty())
          {
            THROW(utils::ValueException,
                  "Expected version to be 1.0 or 1.1. Provided: ", version);
          }

          if (type == QuantumUtil::Problem_ProblemType_ISING ||
              type == QuantumUtil::Problem_ProblemType_PUBO)
          {
            if (cost_function.terms_size() == 0)
            {
              THROW(utils::ValueException,
                    "Problem terms cannot be 0. Please "
                    "add problem terms");
            }
          }

          handler_proxy.StartObject();  // Start the version, type, terms and
                                        // (optionally init config)
          handler_proxy.Key("type");
          handler_proxy.String(model_type[type]);
          handler_proxy.Key("version");
          handler_proxy.String(version);
          handler_proxy.Key("terms");
          handler_proxy.StartArray();  // Start the terms array
        }
        Parse(terms, handler_proxy, term_count);
        problem.Clear();
        file_count++;
        file_handler.close();
        file_name = folder_path + "/" + folder_name + "_" +
                    std::to_string(file_count) + ".pb";
        file_handler.open(file_name, std::ios::in | std::ios::binary);
      }
      else
      {
        throw ConfigurationException(
            "Invalid problem message. No cost function found",
            utils::Error::MissingInput);
      }
    } while (file_handler);

    file_handler.close();
    handler_proxy.EndArray(term_count);  // End terms array;
    handler_proxy.EndObject(
        4);  // End reading terms, version and type and init config
    handler_proxy.EndObject(1);  // End reading the cost function
    handler_proxy.EndObject(
        file_count);  // End reading all files in problem directory
    return handler_proxy.complete();
  }

  // Parsing a vector of Terms. Akin to a Graph
  template <typename StreamHandler>
  bool Parse(const google::protobuf::RepeatedPtrField<
                 QuantumUtil::Problem_Term>& terms,
             utils::PROTOHandlerProxy<StreamHandler>& handler_proxy,
             unsigned& num_terms)
  {
    unsigned term_size = terms.size();
    bool parse_res = false;

    for (unsigned i = 0; i < term_size; i++)
    {
      QuantumUtil::Problem_Term term = terms.Get(i);
      parse_res &= Parse(term, handler_proxy);
    }
    num_terms += term_size;
    return parse_res;
  }

  // Parsing a single term. Akin to an Edge
  template <typename StreamHandler>
  bool Parse(const QuantumUtil::Problem_Term& term,
             utils::PROTOHandlerProxy<StreamHandler>& handler_proxy)
  {
    uint32_t id_size = term.ids_size();
    handler_proxy.StartObject();
    handler_proxy.Key("c");
    handler_proxy.Double(term.c());
    handler_proxy.Key("ids");
    handler_proxy.StartArray();
    for (unsigned i = 0; i < id_size; i++)
    {
      handler_proxy.Int(term.ids(i));
    }

    return handler_proxy.EndObject(
        2);  // ends the fields c and id in a term object.
  }
};

////////////////////////////////////////////////////////////////////////////////
/// Stream configure from PROTO folder.
/// 'Type' is the type of value to configure
/// 'Handler' is the type of stream handler for 'Type'.
/// By default use 'StreamHandler' defined in 'Type'.
///
template <typename Type, typename Handler = typename Type::StreamHandler>
void configure_from_proto_folder(const std::string& folder_name, Type& val)
{
  utils::PROTOHandlerProxy<Handler> handler_proxy;
  utils::ProtoReader reader;
  bool parse_res = reader.Parse(folder_name, handler_proxy);
  if (!parse_res)
  {
    if (!handler_proxy.complete())
    {
      throw ConfigurationException(handler_proxy.error_message(),
                                   handler_proxy.error_code());
    }
  }
  val = std::move(handler_proxy.get_value());
}

////////////////////////////////////////////////////////////////////////////////
/// Stream configure from proto folder.
/// Intermidiate configuration object is created and used.
/// 'Type' is the type of value to configure
/// 'ConfigurationType' is the type of configuration object for 'Type'.
/// By default 'Configuration_T' defined in 'Type' is used.
///
template <class Type,
          typename ConfigurationType = typename Type::Configuration_T>
void configure_with_configuration_from_proto_folder(
    const std::string& folder_name, Type& val)
{
  ConfigurationType configuration;
  configure_from_proto_folder(folder_name, configuration);
  val.configure(configuration);
}
}  // namespace utils

// Copyright 2016 Yahoo Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
#pragma once

#include <string>
#include <vector>
#include <vespa/filedistribution/common/buffer.h>
#include <vespa/filedistribution/common/exception.h>

namespace filedistribution {

struct InvalidProgressException : public Exception {
    const char* what() const throw() {
        return "Invalid progress information reported by one of the filedistributors";
    }
};

struct FileDoesNotExistException : public Exception {};

class FileDBModel : boost::noncopyable {
public:
    class InvalidHostStatusException : public Exception {};
    struct HostStatus {
        enum State { finished, inProgress, notStarted };

        State _state;
        size_t _numFilesToDownload;
        size_t _numFilesFinished;
    };

    virtual ~FileDBModel();

    virtual bool hasFile(const std::string& fileReference) = 0;
    virtual void addFile(const std::string& fileReference, const Buffer& buffer) = 0;
    virtual Move<Buffer> getFile(const std::string& fileReference) = 0;
    virtual void cleanFiles(const std::vector<std::string>& filesToPreserve) = 0;

    virtual void setDeployedFilesToDownload(const std::string& hostName,
            const std::string & appId,
            const std::vector<std::string> & files) = 0;
    virtual void cleanDeployedFilesToDownload(
            const std::vector<std::string> & hostsToPreserve,
            const std::string& appId) = 0;
    virtual void removeDeploymentsThatHaveDifferentApplicationId(
            const std::vector<std::string> & hostsToPreserve,
            const std::string& appId) = 0;
    virtual std::vector<std::string> getHosts() = 0;

    virtual HostStatus getHostStatus(const std::string& hostName) = 0;
    //TODO: does not really belong here, refactor.
    typedef std::vector<int8_t> Progress; // [0-100]
    virtual Progress getProgress(const std::string& fileReference,
                                 const std::vector<std::string>& hostsSortedAscending) = 0;
};

} //namespace filedistribution


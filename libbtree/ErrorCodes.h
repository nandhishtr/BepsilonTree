#pragma once

enum class ErrorCode {
    Success,
    Error,
    ChildSplitCalledOnLeafNode,
    KeyDoesNotExist,
};

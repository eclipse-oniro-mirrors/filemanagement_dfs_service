/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "iremote_broker.h"
#include "message_parcel_mock.h"

namespace OHOS {
using namespace OHOS::Storage::DistributedFile;

Parcelable::Parcelable() : Parcelable(false)
{}

Parcelable::Parcelable(bool asRemote)
{
    asRemote_ = asRemote;
    behavior_ = 0;
}

bool MessageParcel::WriteInterfaceToken(std::u16string name)
{
    return DfsMessageParcel::messageParcel->WriteInterfaceToken(name);
}

std::u16string MessageParcel::ReadInterfaceToken()
{
    return DfsMessageParcel::messageParcel->ReadInterfaceToken();
}

bool Parcel::WriteParcelable(const Parcelable *object)
{
    return DfsMessageParcel::messageParcel->WriteParcelable(object);
}

bool Parcel::WriteInt32(int32_t value)
{
    return DfsMessageParcel::messageParcel->WriteInt32(value);
}

int32_t Parcel::ReadInt32()
{
    return DfsMessageParcel::messageParcel->ReadInt32();
}

bool Parcel::ReadInt32(int32_t &value)
{
    return DfsMessageParcel::messageParcel->ReadInt32(value);
}

bool Parcel::WriteRemoteObject(const Parcelable *object)
{
    return DfsMessageParcel::messageParcel->WriteRemoteObject(object);
}

bool MessageParcel::WriteRemoteObject(const sptr<IRemoteObject> &object)
{
    return DfsMessageParcel::messageParcel->WriteRemoteObject(object);
}

sptr<IRemoteObject> MessageParcel::ReadRemoteObject()
{
    return DfsMessageParcel::messageParcel->ReadRemoteObject();
}

bool Parcel::ReadBool()
{
    return DfsMessageParcel::messageParcel->ReadBool();
}

bool Parcel::ReadBool(bool &value)
{
    return DfsMessageParcel::messageParcel->ReadBool(value);
}

bool Parcel::WriteBool(bool value)
{
    return DfsMessageParcel::messageParcel->WriteBool(value);
}

bool Parcel::WriteString(const std::string &value)
{
    return DfsMessageParcel::messageParcel->WriteString(value);
}

bool Parcel::WriteCString(const char *value)
{
    return DfsMessageParcel::messageParcel->WriteCString(value);
}

bool Parcel::ReadString(std::string &value)
{
    return DfsMessageParcel::messageParcel->ReadString(value);
}

bool Parcel::ReadStringVector(std::vector<std::string> *value)
{
    return DfsMessageParcel::messageParcel->ReadStringVector(value);
}

bool MessageParcel::WriteFileDescriptor(int fd)
{
    return DfsMessageParcel::messageParcel->WriteFileDescriptor(fd);
}

int MessageParcel::ReadFileDescriptor()
{
    return DfsMessageParcel::messageParcel->ReadFileDescriptor();
}

bool Parcel::ReadUint32(uint32_t &value)
{
    return DfsMessageParcel::messageParcel->ReadUint32(value);
}

bool Parcel::WriteUint64(uint64_t value)
{
    return DfsMessageParcel::messageParcel->WriteUint64(value);
}

bool Parcel::WriteUint16(uint16_t value)
{
    return DfsMessageParcel::messageParcel->WriteUint16(value);
}

bool Parcel::ReadUint64(uint64_t &value)
{
    return DfsMessageParcel::messageParcel->ReadUint64(value);
}

bool Parcel::WriteStringVector(const std::vector<std::string> &val)
{
    return DfsMessageParcel::messageParcel->WriteStringVector(val);
}

bool Parcel::WriteUint32(uint32_t value)
{
    if (DfsMessageParcel::messageParcel) {
        return DfsMessageParcel::messageParcel->WriteUint32(value);
    }
    std::cout << "WriteUint32 mock failed, messageParcel is nullptr" << std::endl;
    return false;
}

bool MessageParcel::WriteRawData(const void *data, size_t size)
{
    if (DfsMessageParcel::messageParcel) {
        return DfsMessageParcel::messageParcel->WriteRawData(data, size);
    }
    std::cout << "WriteRawData mock failed, messageParcel is nullptr" << std::endl;
    return false;
}

const void *MessageParcel::ReadRawData(size_t size)
{
    if (DfsMessageParcel::messageParcel) {
        return DfsMessageParcel::messageParcel->ReadRawData(size);
    }
    std::cout << "ReadRawData mock failed, messageParcel is nullptr" << std::endl;
    return nullptr;
}

uint32_t Parcel::ReadUint32()
{
    if (DfsMessageParcel::messageParcel) {
        return DfsMessageParcel::messageParcel->ReadUint32();
    }
    std::cout << "ReadUint32 mock failed, messageParcel is nullptr" << std::endl;
    return 1; // 1: default value
}
} // namespace OHOS
#include <w2rp/changeFromWriter.hpp>
#include <w2rp/log.hpp>

namespace w2rp {

ChangeFromWriter::ChangeFromWriter(
    CacheChange &change, 
    std::uint32_t readerID
):
    readerID(readerID),
    sequenceNumber(change.sequenceNumber),
    lastReceivedFN(0),
    highestFNreceived(0),
    complete(false),
    published(false),
    bitmap(change.numberFragments)
{}

void ChangeFromWriter::markReceived(std::uint32_t fragmentNumber)
{
    lastReceivedFN = fragmentNumber;
    if(fragmentNumber > highestFNreceived)
    {
        highestFNreceived = fragmentNumber;
    }
    if(bitmap.unset(fragmentNumber) == FragmentBitset::Completeness::Complete)
    {
        complete = true;
    }
}

std::uint8_t* ChangeFromWriter::getMissingFragmentMap()
{
    return bitmap.getBitsetData(bitmap.getNextUnsent());
}

std::uint32_t ChangeFromWriter::getMissingFragmentMapBase()
{
    return bitmap.getBitsetBase(bitmap.getNextUnsent());
}

std::uint32_t ChangeFromWriter::getFirstMissingFragment()
{
    return bitmap.getNextUnsent();
}

std::uint32_t ChangeFromWriter::getFragmentCount()
{
    return bitmap.getSizeBits();
}

bool ChangeFromWriter::isComplete()
{
    return complete;
}

bool ChangeFromWriter::isPublished()
{
    return published;
}

void ChangeFromWriter::setPublished()
{
    published = true;
}

}; // end namespace
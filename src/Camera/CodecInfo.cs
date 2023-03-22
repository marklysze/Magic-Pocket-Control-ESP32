using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static BMDCore.CCU.CCUPacketTypes;

namespace BMDCore.Camera
{
    class CodecInfo
    {
        public BasicCodec basicCodec = BasicCodec.RAW;
        public byte codecVariant = CodecVariants.kDefault;
    }
}

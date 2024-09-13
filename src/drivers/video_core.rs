use crate::drivers::mailbox::*;

enum VideoCoreMailboxTagIdentifier {
    SetPower = 0x28001,
    SetClockRate = 0x38002,
    SetPhysicalWidthHeight = 0x48003,
    SetVirtualWidthHeight = 0x48004,
    SetVirtualOffset = 0x48009,
    SetDepth = 0x48005,
    SetPixelOrder = 0x48006,
    GetFrameBuffer = 0x40001,
    GetPitch = 0x40008,
}

pub struct VideoCoreDriver {
    mailbox: Mailbox,
    channel: u32,
    width: i32,
    height: i32,
    depth: i32,
    pitch: i32,
    framebuffer: *mut u8,
}

impl VideoCoreDriver {
    pub fn new(addr: *mut u32, mailbox_channel: u32) -> VideoCoreDriver {

        let mailbox = Mailbox::new(addr);
        let channel = mailbox_channel;        

        let mut driver = VideoCoreDriver {
            mailbox,
            channel,
            width: 0,
            height: 0,
            depth: 0,
            pitch: 0,
            framebuffer: core::ptr::null_mut(),
        };

        driver.init_frame_buffer();

        driver
    }

    fn init_frame_buffer(&mut self) {
        let mut prop_init = MailboxPropertiesBuffer::new();

        prop_init.add_tag(VideoCoreMailboxTagIdentifier::SetPhysicalWidthHeight as u32, &[800, 600]);
        let off_virt_wh = prop_init.add_tag(VideoCoreMailboxTagIdentifier::SetVirtualWidthHeight as u32, &[800, 600]);
        prop_init.add_tag(VideoCoreMailboxTagIdentifier::SetVirtualOffset as u32, &[0, 0]);
        let off_depth = prop_init.add_tag(VideoCoreMailboxTagIdentifier::SetDepth as u32, &[32]);
        prop_init.add_tag(VideoCoreMailboxTagIdentifier::SetPixelOrder as u32, &[1]);
        let off_fb = prop_init.add_tag(VideoCoreMailboxTagIdentifier::GetFrameBuffer as u32, &[4096, 0]);
        let off_pitch = prop_init.add_tag(VideoCoreMailboxTagIdentifier::GetPitch as u32, &[0]);

        prop_init.finish();

        
        self.mailbox.write_properties(&mut prop_init, self.channel);

        let buffer = prop_init.get_buffer();
        
        self.width = buffer[off_virt_wh + 3] as i32;
        self.height = buffer[off_virt_wh + 4] as i32;
        self.depth = buffer[off_depth + 3] as i32;
        self.pitch = buffer[off_pitch + 3] as i32;
        self.framebuffer = (buffer[off_fb + 3] & 0x3FFFFFFF) as *mut u8;
    }

    pub fn get_mut_framebuffer(&mut self) -> *mut u8 {
        self.framebuffer
    }

    pub fn get_framebuffer(&self) -> *const u8 {
        self.framebuffer
    }

    pub fn get_width(&self) -> i32 {
        self.width
    }

    pub fn get_height(&self) -> i32 {
        self.height
    }

    pub fn get_depth(&self) -> i32 {
        self.depth
    }

    pub fn get_pitch(&self) -> i32 {
        self.pitch
    }
}



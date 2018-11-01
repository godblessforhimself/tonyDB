class RID {
	public:
	    RID();                        // Default constructor
	    ~RID();                        // Destructor
	    RID(PageNum pageNum, SlotNum slotNum); // Construct RID from page and slot number
	    RC GetPageNum(PageNum &pageNum) const;  // Return page number
	    RC GetSlotNum(SlotNum &slotNum) const;  // Return slot number
};
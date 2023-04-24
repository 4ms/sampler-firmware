#pragma once
enum Animations {
	ANI_WAITING,
	ANI_WRITING,
	ANI_RECEIVING,
	ANI_SYNC,
	ANI_DONE,
	ANI_SUCCESS,
	ANI_FAIL_ERR,
	ANI_FAIL_SYNC,
	ANI_FAIL_CRC,
	ANI_RESET
};

void animate(Animations animation_type);
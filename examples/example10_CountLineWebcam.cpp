/*
���̃T���v���ł́A��ʏ�Ɉ����������̏�����l�̐l���ǂ̕����Ɉړ����������J�E���g���܂��B
example08_CountLine.cpp�Ƃ̈Ⴂ�͓��͂ɓ���ł͂Ȃ�Web�J�������g�p���Ă���_�ł��B
��͌��ʂ� openpose_ext/build/bin/media �̒��ɋL�^�J�n�����̃t�@�C�����ŕۑ�����܂��B
*/

#include <OpenPoseWrapper/MinimumOpenPose.h>
#include <Utils/Video.h>
#include <Utils/Preview.h>
#include <Utils/VideoControllerUI.h>
#include <Utils/PlotInfo.h>
#include <Utils/SqlOpenPose.h>
#include <Utils/Tracking.h>
#include <Utils/PeopleCounter.h>
#include <time.h>
#include <thread>

int main(int argc, char* argv[])
{
	/*
	example10_CountLineWebcam���R�}���h���C������ĂԂƂ��̃���

	�R�}���h���C�������̐���
			example10_CountLineWebcam <rtmp��URL> <�����̊J�nX���W> <�����̊J�nY���W><�����̏I��X���W> <�����̏I��Y���W> <�����̑���>

			��: example10_CountLineWebcam "rtmp://10.0.0.1/live/guest001" 0 240 640 240 5

	�Ȃ��A�����̎w����ȗ�����ƃf�t�H���g�̒l���g�p�����
	rtmp��URL���ȗ�����ƁAUSB�ڑ�����Ă���Web�J�������g�p�����
	*/

	cv::VideoCapture webcam;
	int startX = 0, startY = 240, endX = 1920, endY = 240, lineWeight = 0;
	if (argc >= 2) {
		// �R�}���h���C�������̑�1�����ɃJ������URL���w��ł���
		webcam.open(argv[1]);
		if (argc >= 7) {
			// �R�}���h���C�������̑�2����: �����̊J�n�n�_��x���W
			// �R�}���h���C�������̑�3����: �����̊J�n�n�_��y���W
			// �R�}���h���C�������̑�4����: �����̏I���n�_��x���W
			// �R�}���h���C�������̑�5����: �����̏I���n�_��y���W
			startX = atoi(argv[2]);
			startY = atoi(argv[3]);
			endX =   atoi(argv[4]);
			endY =   atoi(argv[5]);
			lineWeight = atoi(argv[6]);
		}
	}
	else {
		webcam.open(0);
		/*
		Windows�ŋN�������s����ꍇ
			webcam.open(0);
		�̍s��
			webcam.open(cv::CAP_DSHOW + 0);
		�ɏ���������Ǝ��邩������Ȃ��ł��B
		*/
	}

	if (!webcam.isOpened()) {
		std::cout << "failed to open the web camera" << std::endl;
		return 0;
	}

	// ���ݎ������擾���� (�t�@�C�����Ɏg�p����)
	time_t timer = time(NULL); tm ptm;
	localtime_s(&ptm, &timer);
	char time_c_str[256] = { '\0' }; strftime(time_c_str, sizeof(time_c_str), "%Y-%m-%d_%H-%M-%S", &ptm);
	std::string time(time_c_str);
	std::cout << time << std::endl;

	// �o�͂��� SQL �t�@�C���̃t���p�X��^��J�n�����ɂ���
	std::string sqlPath = R"(media/webcam_)" + time + R"(.sqlite3)";

	// OpenPose �̏�����������
	MinOpenPose openpose(op::PoseModel::BODY_25, op::Point<int>(-1, 368));

	// ������v���r���[���邽�߂̃E�B���h�E�𐶐�����
	Preview preview("result");

	// SQL �̓ǂݏ������s���N���X�̏�����
	SqlOpenPose sql;
	sql.open(sqlPath, 300);

	// ���i���g���b�L���O����N���X
	Tracking tracker(
		0.5f,  // �֐߂̐M���l�����̒l�ȉ��ł���ꍇ�́A�֐߂����݂��Ȃ����̂Ƃ��ď�������
		5,     // �M���l��confidenceThreshold���傫���֐߂̐������̒l�����ł���ꍇ�́A���̐l�����Ȃ����̂Ƃ��ď�������
		10,    // ��x�g���b�L���O���O�ꂽ�l�����̃t���[�������o�߂��Ă��Ĕ�������Ȃ��ꍇ�́A�����������̂Ƃ��ď�������
		150.0f  // �g���b�L���O���̐l��1�t���[���i�񂾂Ƃ��A�ړ����������̒l�����傫���ꍇ�͓���l���̌�₩��O��
	);

	// �ʍs�l���J�E���g����N���X
	PeopleCounter count(
		startX, startY,    // �����̎n�_���W (X, Y)
		endX  , endY  ,  // �����̏I�_���W (X, Y)
		lineWeight         // �����̑���
	);

	cv::Mat image, image2;
	bool exitFlag = false;
	// ����̎��̃t���[����ǂݍ���
	if (!webcam.read(image2)) return 0;
	// �ʃX���b�h�œ����ǂݍ���
	std::mutex mtx;
	std::thread th([&]() {
		while (true) {
			std::scoped_lock{ mtx };
			if (exitFlag) break;
			if (!webcam.read(image2)) break;
			if (image2.empty()) break;
		}
	});

	// ���悪�I���܂Ń��[�v����
	uint64_t frameNumber = 0;
	while (true)
	{
		// �ʃX���b�h�œǂݍ��񂾓�����R�s�[
		{
			std::scoped_lock{ mtx };
			image = image2.clone();
		}

		// �f�����I�������ꍇ�̓��[�v�𔲂���
		if (image.empty()) break;

		// �p������
		MinOpenPose::People people = openpose.estimate(image);

		// �g���b�L���O
		auto tracked_people = tracker.tracking(people, sql, frameNumber).value();

		// �ʍs�l�̃J�E���g
		count.update(tracker, frameNumber);

		// �ʍs�l�̃J�E���g�󋵂��v���r���[
		count.drawInfo(image, tracker);

		// �p������̌��ʂ� image �ɕ`�悷��
		plotBone(image, tracked_people, openpose);


		cv::resize(image, image, cv::Size(640, 480) );

		// ��ʂ��X�V����
		int ret = preview.preview(image);

		// Esc�L�[�������ꂽ��I������
		if (0x1b == ret) break;

		frameNumber += 1;
	}

	// �X���b�h�̏I���t���O�𗧂Ă�
	{
		std::scoped_lock{ mtx };
		exitFlag = true;
	}

	// �X���b�h�̏I����҂�
	th.join();

	return 0;
}

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

using namespace std;

int main(int argc, char *argv[])
{
    ///////////////////////// Demux using FFmpeg /////////////////////////
    // FFmpeg Demux
    const char *filename = argv[1];
    AVFormatContext *file_context = nullptr;

    // open input file
    avformat_open_input(&file_context, filename, nullptr, nullptr);

    // find stream
    avformat_find_stream_info(file_context, nullptr);
    av_dump_format(file_context, 0, filename, 0);

    // find video stream
    AVStream *video_stream = nullptr;
    int video_stream_index = -1;
    for (unsigned int i = 0; i < file_context->nb_streams; i++)
    {
        if (file_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream = file_context->streams[i];
            video_stream_index = i;
            break;
        }
    }

    // find audio stream
    AVStream *audio_stream = nullptr;
    int audio_stream_index = -1;
    for (unsigned int i = 0; i < file_context->nb_streams; i++)
    {
        if (file_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_stream = file_context->streams[i];
            audio_stream_index = i;
            break;
        }
    }

    ///////////////////////// Prepare Decoder /////////////////////////
    // video's codec
    AVCodecParameters *file_codec_parameters = video_stream->codecpar;
    const AVCodec *video_codec = avcodec_find_decoder(file_codec_parameters->codec_id);
    AVCodecContext *decoder_context = avcodec_alloc_context3(video_codec);
    avcodec_parameters_to_context(decoder_context, file_codec_parameters);
    avcodec_open2(decoder_context, video_codec, nullptr);

    ///////////////////////// Prepare Encoder /////////////////////////
    const AVCodec *encoder_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVCodecContext *encoder_context = avcodec_alloc_context3(encoder_codec);
    encoder_context->height = 360;
    encoder_context->width = 720;
    encoder_context->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_context->time_base = video_stream->time_base;
    avcodec_open2(encoder_context, encoder_codec, nullptr);

    // create container
    AVFormatContext *output_context = nullptr;
    avformat_alloc_output_context2(&output_context, nullptr, nullptr, "output.mp4");

    // video output stream
    AVStream *video_out_stream = avformat_new_stream(output_context, nullptr);
    avcodec_parameters_from_context(video_out_stream->codecpar, encoder_context);
    video_out_stream->time_base = encoder_context->time_base;

    // audio output stream
    AVStream *audio_out_stream = avformat_new_stream(output_context, nullptr);
    avcodec_parameters_copy(audio_out_stream->codecpar, audio_stream->codecpar);
    audio_out_stream->time_base = audio_stream->time_base;

    avio_open(&output_context->pb, "output.mp4", AVIO_FLAG_WRITE);
    avformat_write_header(output_context, nullptr);

    ///////////////////////// Frame Scaler /////////////////////////
    SwsContext *scale_context = sws_getContext(
        decoder_context->width, decoder_context->height, decoder_context->pix_fmt,
        encoder_context->width, encoder_context->height, encoder_context->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *modified_frame = av_frame_alloc();
    modified_frame->format = encoder_context->pix_fmt;
    modified_frame->width = encoder_context->width;
    modified_frame->height = encoder_context->height;
    av_frame_get_buffer(modified_frame, 32);

    ///////////////////////// Decode -> Scale -> Encode /////////////////////////
    while (av_read_frame(file_context, packet) >= 0)
    {
        if (packet->stream_index == video_stream_index)
        {
            avcodec_send_packet(decoder_context, packet);
            while (avcodec_receive_frame(decoder_context, frame) == 0)
            {
                // Modify frame
                sws_scale(scale_context, frame->data, frame->linesize, 0, decoder_context->height, modified_frame->data, modified_frame->linesize);
                modified_frame->pts = frame->pts;

                // Encode frame
                avcodec_send_frame(encoder_context, modified_frame);
                AVPacket *output_packet = av_packet_alloc();
                while (avcodec_receive_packet(encoder_context, output_packet) == 0)
                {
                    av_interleaved_write_frame(output_context, output_packet);
                    av_packet_unref(output_packet);
                }
                av_packet_free(&output_packet);
            }
        }
        else if (packet->stream_index == audio_stream_index && audio_out_stream)
        {
            AVPacket *audio_packet = av_packet_clone(packet);
            audio_packet->stream_index = audio_out_stream->index;
            av_packet_rescale_ts(audio_packet, audio_stream->time_base, audio_out_stream->time_base);
            av_interleaved_write_frame(output_context, audio_packet);
            av_packet_free(&audio_packet);
        }

        av_packet_unref(packet);
    }

    avcodec_send_frame(encoder_context, nullptr);
    AVPacket *output_packet = av_packet_alloc();
    while (avcodec_receive_packet(encoder_context, output_packet) == 0)
    {
        av_interleaved_write_frame(output_context, output_packet);
        av_packet_unref(output_packet);
    }
    av_packet_free(&output_packet);
    av_write_trailer(output_context);

    ///////////////////////// Free FFmpeg resource /////////////////////////
    // free FFmpeg resource

    av_frame_free(&frame);
    av_frame_free(&modified_frame);
    av_packet_free(&packet);
    sws_freeContext(scale_context);

    avcodec_free_context(&decoder_context);
    avcodec_free_context(&encoder_context);
    avformat_close_input(&file_context);

    avio_closep(&output_context->pb);
    avformat_free_context(output_context);

    return 0;
}